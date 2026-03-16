#include "Scheduler.hpp"
#include <vector>
#include <limits>
#include <algorithm>
#include <optional>
#include <cmath>
#include "../alloc/ResourceAllocator.hpp"
#include "../rules/GPTreeRule.hpp"
#include "../rules/GPTreeResRule.hpp"
#include <iostream>
#include <iomanip>
#include <unordered_map>

extern bool g_trace;

static const std::vector<float>* g_priorityKeys = nullptr;
static const std::vector<int>* g_forcedResource = nullptr;
static const std::unordered_map<int, int>* g_resIndex = nullptr;
static const std::unordered_map<std::string, std::vector<int>>* g_skillLevels = nullptr;


void Scheduler::setPriorityKeys(const std::vector<float>* keys) { g_priorityKeys = keys; }
void Scheduler::setForcedResources(const std::vector<int>* forced) { g_forcedResource = forced; }


static inline int skillLevelOf(const Instance& I, int resId, const std::string& skill) {
    int idx = -1;
    if (g_resIndex) {
        auto it = g_resIndex->find(resId);
        if (it != g_resIndex->end()) idx = it->second;
    }
    if (idx < 0) {
        auto it = std::find_if(I.resources.begin(), I.resources.end(),
            [&](const Resource& r) { return r.id == resId; });
        if (it == I.resources.end()) return 0;
        idx = int(it - I.resources.begin());
    }

    if (g_skillLevels) {
        auto it = g_skillLevels->find(skill);
        if (it != g_skillLevels->end()) {
            const auto& v = it->second;
            return v[idx];
        }
    }

    const auto& r = I.resources[idx];
    auto jt = r.skills.find(skill);
    if (jt == r.skills.end()) return 0;
    return jt->second;
}

static inline const Resource* findRes(const Instance& I, int resId) {
    if (g_resIndex) {
        auto it = g_resIndex->find(resId);
        if (it != g_resIndex->end()) return &I.resources[it->second];
    }
    auto it = std::find_if(I.resources.begin(), I.resources.end(),
        [&](const Resource& r) { return r.id == resId; });
    return (it == I.resources.end() ? nullptr : &*it);
}

static inline bool isFreeNow(const Instance& I, int resId, int now) {
    if (g_resIndex) {
        auto it = g_resIndex->find(resId);
        if (it != g_resIndex->end()) return (I.resources[it->second].busyUntil <= now);
    }
    if (auto* r = findRes(I, resId)) return (r->busyUntil <= now);
    return false;
}



static std::optional<std::vector<int>> tryAllocWithForced(
    const Instance& I, const Task& t, int now, int forcedResId)
{
    if (forcedResId < 0) return std::nullopt;
    if (!isFreeNow(I, forcedResId, now)) return std::nullopt;

    if (t.reqLevel > 0) {
        int lvl = skillLevelOf(I, forcedResId, t.reqSkill);
        if (lvl < t.reqLevel) return std::nullopt;
    }
    return std::vector<int>{ forcedResId };
}


static std::optional<std::vector<int>> tryAllocZeroReqWithForced(
    const Instance& I, const Task& t, int now, int forcedResId)
{
    return tryAllocWithForced(I, t, now, forcedResId);
}

ScheduleResult Scheduler::precedenceOnly(Instance& I, const IDispatchingRule& rule)
{
    const int n = (int)I.tasks.size();

    std::vector<int> indeg(n, 0);
    for (int i = 0; i < n; ++i)
        for (int pid : I.tasks[i].predecessors)
            if (I.idToIndex.count(pid)) indeg[i]++;

    for (auto& t : I.tasks) { t.start = t.finish = -1; t.assignedResources.clear(); }

    int scheduled = 0, globalFinish = 0;

    while (scheduled < n) {
        std::vector<int> cand;
        for (int i = 0; i < n; ++i)
            if (indeg[i] == 0 && I.tasks[i].start == -1) cand.push_back(i);

        if (cand.empty()) break;

        int bestIx = -1; double bestScore = std::numeric_limits<double>::infinity();
        for (int ix : cand) {
            double s = rule.score(I.tasks[ix]);
            if (s < bestScore) { bestScore = s; bestIx = ix; }
        }

        int est = 0;
        for (int pid : I.tasks[bestIx].predecessors)
            if (I.idToIndex.count(pid))
                est = std::max(est, I.tasks[I.idToIndex[pid]].finish);

        I.tasks[bestIx].start = est;
        I.tasks[bestIx].finish = est + I.tasks[bestIx].duration;
        globalFinish = std::max(globalFinish, I.tasks[bestIx].finish);

        const int finishedId = I.tasks[bestIx].id;
        for (int j = 0; j < n; ++j) if (I.tasks[j].start == -1)
            for (int pid : I.tasks[j].predecessors)
                if (pid == finishedId) indeg[j]--;

        scheduled++;
    }
    return { globalFinish, 0.0 };
}

ScheduleResult Scheduler::withResources(Instance& I,
    const IDispatchingRule& ruleT,
    const GPTreeResRule* ruleR)
{
    const int n = (int)I.tasks.size();

    std::unordered_map<int, int> __resIndex;
    __resIndex.reserve(I.resources.size() * 2);
    for (int i = 0; i < (int)I.resources.size(); ++i)
        __resIndex[I.resources[i].id] = i;
    g_resIndex = &__resIndex;

    std::unordered_map<std::string, std::vector<int>> __skillLevels;
    __skillLevels.reserve(16);
    for (size_t ri = 0; ri < I.resources.size(); ++ri) {
        const auto& r = I.resources[ri];
        for (const auto& kv : r.skills) {
            if (kv.second <= 0) continue;
            auto& vec = __skillLevels[kv.first];
            if (vec.empty()) vec.assign(I.resources.size(), 0);
            vec[ri] = kv.second;
        }
    }
    g_skillLevels = &__skillLevels;

    std::unordered_map<std::string, std::vector<int>> __resBySkill;
    __resBySkill.reserve(16);
    for (const auto& r : I.resources) {
        for (const auto& kv : r.skills) {
            __resBySkill[kv.first].push_back(r.id);
        }
    }

    ResourceAllocator::setPrecomputed(&__resBySkill, &__resIndex, &__skillLevels);

    std::vector<int> indeg(n, 0);
    for (int i = 0; i < n; ++i)
        for (int pid : I.tasks[i].predecessors)
            if (I.idToIndex.count(pid)) indeg[i]++;

    for (auto& t : I.tasks) { t.start = -1; t.finish = -1; t.assignedResources.clear(); }
    for (auto& r : I.resources) { r.busy = false; r.busyUntil = 0; }

    auto freeResources = [&](int now) {
        for (auto& r : I.resources) if (r.busyUntil <= now) r.busy = false;
        };

    int now = 0, scheduled = 0, makespan = 0;
    double totalCost = 0.0;

    struct Running { int ix; int finish; std::vector<int> res; };
    std::vector<Running> running; running.reserve(n);

    if (g_trace) {
        size_t szK = g_priorityKeys ? g_priorityKeys->size() : 0;
        size_t szF = g_forcedResource ? g_forcedResource->size() : 0;
        if (szK && szK < I.tasks.size())
            std::cout << "[warn] priorityKeys size=" << szK
            << " < tasks=" << I.tasks.size() << "\n";
        if (szF && szF < I.tasks.size())
            std::cout << "[warn] forcedResource size=" << szF
            << " < tasks=" << I.tasks.size() << "\n";
    }

    while (scheduled < n) {
        freeResources(now);

        std::vector<int> cand;
        for (int i = 0; i < n; ++i)
            if (indeg[i] == 0 && I.tasks[i].start == -1) cand.push_back(i);

        bool startedAny = false;
        while (!cand.empty()) {
            const_cast<IDispatchingRule&>(ruleT).setContext(&I, now);

            const GPTreeRule* gp = nullptr;
            if (g_trace) {
                gp = dynamic_cast<const GPTreeRule*>(&ruleT);
                if (gp) {
                    std::cout << "\n[time now=" << now << "]  GP = " << gp->exprString() << "\n";
                    std::cout << "ID  DUR  REQ  AVAIL  GAP  WAIT  EST  CRITLEN  SLACK  SUCC  TPRED   SCORE\n";
                }
                else {
                    std::cout << "\n[time now=" << now << "]  (trace dostępny tylko dla GPTreeRule)\n";
                }
            }

            int best = -1;
            double bestScore = std::numeric_limits<double>::infinity();
            std::vector<int> bestSet;
            int minWaitFeasible = std::numeric_limits<int>::max() / 4;

            for (int ix : cand) {
                Task& t = I.tasks[ix];
                const int req = t.reqLevel;

                int forcedId = -1;
                if (g_forcedResource && (size_t)ix < g_forcedResource->size())
                    forcedId = (*g_forcedResource)[ix];

                std::optional<std::vector<int>> allocSet;

                if (forcedId >= 0) {
                    if (req <= 0) {
                        allocSet = tryAllocZeroReqWithForced(I, t, now, forcedId);
                        if (!allocSet) {
                            if (auto* r = findRes(I, forcedId)) {
                                int w = std::max(0, r->busyUntil - now);
                                minWaitFeasible = std::min(minWaitFeasible, w);
                            }
                        }
                    }
                    else {
                        allocSet = tryAllocWithForced(I, t, now, forcedId);
                        if (!allocSet) {
                            int waitAll = ResourceAllocator::waitUntilFeasible(I, now, t.reqSkill, req);
                            minWaitFeasible = std::min(minWaitFeasible, waitAll);

                            auto fallback = ResourceAllocator::cheapestSubset(I, t.reqSkill, req, now);
                            if (fallback) {
                                bool containsForced = std::find(fallback->begin(), fallback->end(), forcedId) != fallback->end();
                                if (containsForced) allocSet = fallback;
                            }
                        }
                    }
                }
                else {
                    if (ruleR == nullptr) {
                        allocSet = ResourceAllocator::cheapestSubset(I, t.reqSkill, req, now);
                        if (!allocSet) {
                            int wait = ResourceAllocator::waitUntilFeasible(I, now, t.reqSkill, req);
                            minWaitFeasible = std::min(minWaitFeasible, wait);
                        }
                    }
                    else {
                        std::vector<std::pair<int, double>> scored;
                        scored.reserve(I.resources.size());

                        for (const auto& r : I.resources) {
                            if (r.busyUntil > now) continue;

                            if (req > 0) {
                                int lvl = skillLevelOf(I, r.id, t.reqSkill);
                                if (lvl < req) continue;
                            }

                            double s = ruleR->score(I, t, r, now);
                            scored.emplace_back(r.id, s);
                        }

                        if (scored.empty()) {
                            allocSet = std::nullopt;
                            int wait = ResourceAllocator::waitUntilFeasible(I, now, t.reqSkill, req);
                            minWaitFeasible = std::min(minWaitFeasible, wait);
                        }
                        else {
                            auto best = *std::min_element(scored.begin(), scored.end(),
                                [](const auto& a, const auto& b) { return a.second < b.second; });
                            allocSet = std::vector<int>{ best.first };
                        }
                    }
                }


                if (!allocSet) {
                    if (g_trace && gp) {
                        int avail = ResourceAllocator::availableSkillSum(I, now, t.reqSkill);
                        int wait = ResourceAllocator::waitUntilFeasible(I, now, t.reqSkill, req);
                        std::cout << "T" << t.id
                            << "  " << t.duration
                            << "   " << req
                            << "    " << avail
                            << "    " << (avail - req)
                            << "   " << wait
                            << "   " << 0
                            << "     " << 0
                            << "     " << 0
                            << "    " << 0
                            << "     " << 0
                            << "    " << 1e12 << "  (X)\n";
                    }
                    continue;
                }

                double sc;
                if (gp) {
                    ScoreTrace tr = gp->scoreWithTrace(t);
                    sc = tr.score;
                    if (g_trace) {
                        std::cout << "T" << t.id
                            << "  " << t.duration
                            << "   " << tr.feat.reqLevel
                            << "    " << tr.feat.availSkill
                            << "    " << tr.feat.availGap
                            << "   " << tr.feat.waitRes
                            << "   " << tr.feat.estPrec
                            << "     " << tr.feat.critLen
                            << "     " << tr.feat.slack
                            << "    " << tr.feat.succCount
                            << "     " << tr.feat.totPred
                            << "    " << (std::isfinite(sc) ? sc : 1e12)
                            << "\n";
                    }
                }
                else {
                    sc = ruleT.score(t);
                }

                if (sc < bestScore) {
                    bestScore = sc; best = ix; bestSet = *allocSet;
                }
                else if (best != -1 && std::abs(sc - bestScore) < 1e-9 && g_priorityKeys) {
                    const auto& K = *g_priorityKeys;
                    if ((size_t)best < K.size() && (size_t)ix < K.size()) {
                        if (K[ix] < K[best]) { best = ix; bestSet = *allocSet; }
                    }
                }
            }

            if (g_trace && best != -1) {
                std::cout << "=> wybieram T" << I.tasks[best].id
                    << "  (score=" << bestScore << ")\n";
            }

            if (best == -1) {
                if (minWaitFeasible <= 0) {
                    break;
                }
                if (minWaitFeasible < std::numeric_limits<int>::max() / 8) {
                    now += minWaitFeasible;
                    freeResources(now);
                    continue;
                }
                break;
            }


            {
                Task& t = I.tasks[best];
                t.start = now;
                t.finish = now + t.duration;
                t.assignedResources = bestSet;

                for (int id : bestSet) {
                    for (auto& r : I.resources)
                        if (r.id == id) { r.busy = true; r.busyUntil = t.finish; }
                }

                totalCost += ResourceAllocator::subsetCost(I, bestSet) * double(t.duration);

                running.push_back({ best, t.finish, bestSet });
                makespan = std::max(makespan, t.finish);
                startedAny = true;

                cand.erase(std::remove(cand.begin(), cand.end(), best), cand.end());
                scheduled++;
            }
        }

        if (!startedAny) {
            if (running.empty()) {
                std::cerr << "\n[warn] Brak możliwych startów i brak bieżących zadań. Przerywam.\n";
                break;
            }
            int nextT = std::numeric_limits<int>::max();
            for (auto& rt : running) nextT = std::min(nextT, rt.finish);
            now = nextT;

            std::vector<Running> still; still.reserve(running.size());
            for (auto& rt : running) {
                if (rt.finish == now) {
                    int finishedId = I.tasks[rt.ix].id;
                    for (int j = 0; j < n; ++j) if (I.tasks[j].start == -1)
                        for (int pid : I.tasks[j].predecessors)
                            if (pid == finishedId) indeg[j]--;
                }
                else {
                    still.push_back(rt);
                }
            }
            running.swap(still);

            freeResources(now);
        }
    }
    ResourceAllocator::setPrecomputed(nullptr, nullptr, nullptr);
    g_skillLevels = nullptr;
    g_resIndex = nullptr;
    return { makespan, totalCost };
}

