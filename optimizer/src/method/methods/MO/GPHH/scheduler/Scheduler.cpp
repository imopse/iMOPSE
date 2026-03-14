#include "Scheduler.hpp"
#include <vector>
#include <limits>
#include <algorithm>
#include <optional>
#include <cmath>
#include <string>
#include "../alloc/ResourceAllocator.hpp"
#include "../rules/GPTreeRule.hpp"
#include "../rules/GPTreeResRule.hpp"
#include <iostream>
#include <unordered_map>

extern bool g_trace;

static const std::vector<float>* g_priorityKeys = nullptr;
static const std::vector<int>* g_forcedResource = nullptr;
static const std::unordered_map<int, int>* g_resIndex = nullptr;
static const std::unordered_map<std::string, std::vector<int>>* g_skillLevels = nullptr;

namespace {

    struct ResourceLookupCache {
        std::size_t signature = 0;
        bool ready = false;

        std::unordered_map<int, int> resIndex;
        std::unordered_map<std::string, std::vector<int>> skillLevels;
        std::unordered_map<std::string, std::vector<int>> resBySkill;
    };

    thread_local ResourceLookupCache g_lookupCache;

    static std::size_t getResourceStructureSignature(const Instance& I) {
        if (I.resourceStructureSignatureReady) {
            return I.resourceStructureSignature;
        }

        std::size_t sig = Instance::hashCombine(1469598103934665603ull, I.resources.size());

        for (const auto& r : I.resources) {
            std::size_t resHash = Instance::hashCombine(std::hash<int>{}(r.id), r.skills.size());

            std::size_t skillsHash = 0;
            for (const auto& kv : r.skills) {
                std::size_t pairHash = std::hash<std::string>{}(kv.first);
                pairHash = Instance::hashCombine(pairHash, std::hash<int>{}(kv.second));
                skillsHash ^= Instance::hashCombine(pairHash, 0x517cc1b727220a95ull);
            }

            resHash = Instance::hashCombine(resHash, skillsHash);
            sig = Instance::hashCombine(sig, resHash);
        }

        return sig;
    }

    void rebuildResourceLookupCache(const Instance& I) {
        g_lookupCache.resIndex.clear();
        g_lookupCache.skillLevels.clear();
        g_lookupCache.resBySkill.clear();

        g_lookupCache.resIndex.reserve(I.resources.size() * 2);
        g_lookupCache.skillLevels.reserve(16);
        g_lookupCache.resBySkill.reserve(16);

        for (int i = 0; i < (int)I.resources.size(); ++i) {
            g_lookupCache.resIndex[I.resources[i].id] = i;
        }

        for (size_t ri = 0; ri < I.resources.size(); ++ri) {
            const auto& r = I.resources[ri];

            for (const auto& kv : r.skills) {
                if (kv.second > 0) {
                    auto& vec = g_lookupCache.skillLevels[kv.first];
                    if (vec.empty()) vec.assign(I.resources.size(), 0);
                    vec[ri] = kv.second;
                }

                g_lookupCache.resBySkill[kv.first].push_back(r.id);
            }
        }

        g_lookupCache.signature = getResourceStructureSignature(I);
        g_lookupCache.ready = true;
    }

    void ensureResourceLookupCache(const Instance& I) {
        const std::size_t sig = getResourceStructureSignature(I);

        if (g_lookupCache.ready &&
            g_lookupCache.signature == sig &&
            g_lookupCache.resIndex.size() == I.resources.size()) {
            return;
        }

        rebuildResourceLookupCache(I);
    }

} // namespace

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

static inline bool isImopseCapable(const Task& t, int resId) {
    if (t.capableResources.empty()) return true;
    return std::binary_search(t.capableResources.begin(), t.capableResources.end(), resId);
}

static std::optional<std::vector<int>> cheapestSingleCapableNow(const Instance& I, const Task& t, int now) {
    if (t.capableResources.empty()) return std::nullopt;

    int bestId = -1;
    double bestSalary = std::numeric_limits<double>::infinity();

    for (int rid : t.capableResources) {
        const Resource* r = findRes(I, rid);
        if (!r) continue;
        if (r->salary < bestSalary) { bestSalary = r->salary; bestId = rid; }
    }
    if (bestId < 0) return std::nullopt;
    return std::vector<int>{ bestId };
}

static int waitUntilAnyCapableFree(const Instance& I, const Task& t, int now) {
    if (t.capableResources.empty()) return std::numeric_limits<int>::max() / 4;

    int best = std::numeric_limits<int>::max() / 4;
    for (int rid : t.capableResources) {
        const Resource* r = findRes(I, rid);
        if (!r) continue;
        int w = std::max(0, r->busyUntil - now);
        best = std::min(best, w);
    }
    return best;
}

static std::optional<std::vector<int>> tryAllocWithForced(
    const Instance& I, const Task& t, int now, int forcedResId)
{
    if (forcedResId < 0) return std::nullopt;

    if (!isImopseCapable(t, forcedResId)) return std::nullopt;
    if (!t.capableResources.empty()) {
        return std::vector<int>{ forcedResId };
    }

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

    ensureResourceLookupCache(I);

    const auto& __resIndex = g_lookupCache.resIndex;
    g_resIndex = &__resIndex;
    g_skillLevels = &g_lookupCache.skillLevels;

    ResourceAllocator::setPrecomputed(
        &g_lookupCache.resBySkill,
        &g_lookupCache.resIndex,
        &g_lookupCache.skillLevels
    );

    std::vector<int> indeg(n, 0);
    for (int i = 0; i < n; ++i)
        for (int pid : I.tasks[i].predecessors)
            if (I.idToIndex.count(pid)) indeg[i]++;

    std::vector<std::vector<int>> succ(n);
    for (int j = 0; j < n; ++j) {
        for (int pid : I.tasks[j].predecessors) {
            auto it = I.idToIndex.find(pid);
            if (it != I.idToIndex.end()) {
                succ[it->second].push_back(j);
            }
        }
    }

    std::vector<int> feasibleCountPerTask(n, 0);
    for (int ui = 0; ui < n; ++ui) {
        const Task& u = I.tasks[ui];
        int reqU = std::max(0, u.reqLevel);

        if (reqU <= 0) {
            feasibleCountPerTask[ui] = (int)I.resources.size();
            continue;
        }

        int cnt = 0;
        for (const auto& rr : I.resources) {
            auto jt = rr.skills.find(u.reqSkill);
            int ll = (jt != rr.skills.end()) ? jt->second : 0;
            if (ll >= reqU) ++cnt;
        }
        feasibleCountPerTask[ui] = cnt;
    }

    std::vector<double> staticTaskResCount(n, 0.0);
    std::vector<double> staticAvgResCost(n, std::numeric_limits<double>::infinity());
    for (int ui = 0; ui < n; ++ui) {
        const Task& u = I.tasks[ui];
        const int reqU = std::max(0, u.reqLevel);

        if (reqU <= 0) {
            staticTaskResCount[ui] = (double)I.resources.size();
            staticAvgResCost[ui] = std::numeric_limits<double>::infinity();
            continue;
        }

        if (!u.capableResources.empty()) {
            staticTaskResCount[ui] = (double)u.capableResources.size();

            double sumSal = 0.0;
            int cnt = 0;
            for (int rid : u.capableResources) {
                auto itIdx = __resIndex.find(rid);
                if (itIdx == __resIndex.end()) continue;
                sumSal += I.resources[itIdx->second].salary;
                ++cnt;
            }
            staticAvgResCost[ui] = (cnt > 0) ? (sumSal / (double)cnt) : std::numeric_limits<double>::infinity();
            continue;
        }

        staticTaskResCount[ui] = (double)feasibleCountPerTask[ui];

        double sumSal = 0.0;
        int cnt = 0;
        for (const auto& rr : I.resources) {
            auto jt = rr.skills.find(u.reqSkill);
            const int ll = (jt != rr.skills.end()) ? jt->second : 0;
            if (ll >= reqU) {
                sumSal += rr.salary;
                ++cnt;
            }
        }
        staticAvgResCost[ui] = (cnt > 0) ? (sumSal / (double)cnt) : std::numeric_limits<double>::infinity();
    }

    int unschedCount = n;
    setFeaturePrecomputed(&staticTaskResCount, &staticAvgResCost, &__resIndex, &unschedCount);

    for (auto& t : I.tasks) { t.start = -1; t.finish = -1; t.assignedResources.clear(); }

    std::vector<std::vector<int>> demandCapableResIdxPerTask(n);
    std::vector<double> demandWeightPerTask(n, 0.0);
    std::vector<double> futureDemandByRes(I.resources.size(), 0.0);

    for (int ui = 0; ui < n; ++ui) {
        const Task& u = I.tasks[ui];
        const int reqU = std::max(0, u.reqLevel);
        const double w = 1.0 / (double)std::max(1, feasibleCountPerTask[ui]);
        demandWeightPerTask[ui] = w;

        auto& caps = demandCapableResIdxPerTask[ui];
        if (reqU <= 0) {
            caps.reserve(I.resources.size());
            for (int ri = 0; ri < (int)I.resources.size(); ++ri) {
                caps.push_back(ri);
                futureDemandByRes[ri] += w;
            }
            continue;
        }

        caps.reserve(std::max(1, feasibleCountPerTask[ui]));
        for (int ri = 0; ri < (int)I.resources.size(); ++ri) {
            const auto& rr = I.resources[ri];
            auto jt = rr.skills.find(u.reqSkill);
            const int ll = (jt != rr.skills.end()) ? jt->second : 0;
            if (ll >= reqU) {
                caps.push_back(ri);
                futureDemandByRes[ri] += w;
            }
        }
    }


    for (auto& r : I.resources) {
        r.busy = false;
        r.busyUntil = 0;
        r.busyStart = 0;
        r.totalBusy = 0;
    }

    auto freeResources = [&](int now) {
        for (auto& r : I.resources) {
            if (r.busy && r.busyUntil <= now) {
                int dur = r.busyUntil - r.busyStart;
                if (dur > 0) r.totalBusy += dur;
                r.busy = false;
            }
        }
        };

    int now = 0, scheduled = 0, makespan = 0;
    double totalCost = 0.0;
    std::vector<int> assignedByImopse;
    assignedByImopse.assign(n, -1);

    struct Running { int ix; int finish; std::vector<int> res; };
    std::vector<Running> running; running.reserve(n);

    auto processFinishedAtNow = [&]() {
        std::vector<Running> still;
        still.reserve(running.size());

        for (auto& rt : running) {
            if (rt.finish == now) {
                for (int j : succ[rt.ix]) {
                    if (I.tasks[j].start == -1) indeg[j]--;
                }
            }
            else {
                still.push_back(rt);
            }
        }
        running.swap(still);
        freeResources(now);
        };

    auto advanceTo = [&](int target) {
        while (now < target) {
            if (running.empty()) {
                now = target;
                return;
            }
            int nextT = std::numeric_limits<int>::max();
            for (auto& rt : running) nextT = std::min(nextT, rt.finish);

            if (nextT > target) {
                now = target;
                return;
            }
            now = nextT;
            processFinishedAtNow();
        }
        };

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
                        if (!t.capableResources.empty()) {
                            allocSet = cheapestSingleCapableNow(I, t, now);
                            if (!allocSet) {
                                int wait = waitUntilAnyCapableFree(I, t, now);
                                minWaitFeasible = std::min(minWaitFeasible, wait);
                            }
                        }
                        else {
                            allocSet = ResourceAllocator::cheapestSubset(I, t.reqSkill, req, now);
                            if (!allocSet) {
                                int wait = ResourceAllocator::waitUntilFeasible(I, now, t.reqSkill, req);
                                minWaitFeasible = std::min(minWaitFeasible, wait);
                            }
                        }
                    }
                    else {
                        std::vector<std::pair<int, double>> scored;
                        scored.reserve(I.resources.size());

                        double cheapestNow = std::numeric_limits<double>::infinity();
                        {
                            const int req0 = std::max(0, t.reqLevel);

                            for (const auto& rr : I.resources) {
                                if (rr.busyUntil > now) continue;

                                int lvl = 0;
                                auto it = rr.skills.find(t.reqSkill);
                                if (it != rr.skills.end()) lvl = it->second;

                                if (req0 <= 0 || lvl >= req0) {
                                    cheapestNow = std::min(cheapestNow, rr.salary);
                                }
                            }
                        }

                        if (!t.capableResources.empty()) {
                            scored.reserve(t.capableResources.size());

                            for (int rid : t.capableResources) {
                                auto it = __resIndex.find(rid);
                                if (it == __resIndex.end()) continue;
                                const Resource& r = I.resources[it->second];

                                double futureDemandExcludingTask = futureDemandByRes[it->second] - demandWeightPerTask[ix];
                                if (futureDemandExcludingTask < 0.0) futureDemandExcludingTask = 0.0;

                                double s = ruleR->scoreFast(I, ix, t, r, now, cheapestNow, futureDemandExcludingTask);
                                scored.emplace_back(r.id, s);
                            }
                        }
                        else {
                            scored.reserve(I.resources.size());

                            for (const auto& r : I.resources) {
                                if (req > 0) {
                                    int lvl = skillLevelOf(I, r.id, t.reqSkill);
                                    if (lvl < req) continue;
                                }

                                const int ri = (int)(&r - &I.resources[0]);
                                double futureDemandExcludingTask = futureDemandByRes[ri] - demandWeightPerTask[ix];
                                if (futureDemandExcludingTask < 0.0) futureDemandExcludingTask = 0.0;

                                double s = ruleR->scoreFast(I, ix, t, r, now, cheapestNow, futureDemandExcludingTask);
                                scored.emplace_back(r.id, s);
                            }
                        }

                        if (scored.empty()) {
                            allocSet = std::nullopt;
                            int wait = (!t.capableResources.empty())
                                ? waitUntilAnyCapableFree(I, t, now)
                                : ResourceAllocator::waitUntilFeasible(I, now, t.reqSkill, req);

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

                int desiredStart = now;
                for (int id : bestSet) {
                    auto it = __resIndex.find(id);
                    if (it == __resIndex.end()) continue;
                    desiredStart = std::max(desiredStart, I.resources[it->second].busyUntil);
                }

                bool waited = false;
                if (desiredStart > now) {
                    advanceTo(desiredStart);
                    waited = true;
                }

                t.start = now;
                t.finish = now + t.duration;
                t.assignedResources = bestSet;

                if (!bestSet.empty() && t.imopseIndex >= 0 && t.imopseIndex < (int)assignedByImopse.size()) {
                    assignedByImopse[t.imopseIndex] = bestSet[0];
                }

                for (int id : bestSet) {
                    auto it = __resIndex.find(id);
                    if (it == __resIndex.end()) continue;
                    auto& rr = I.resources[it->second];
                    rr.busy = true;
                    rr.busyStart = t.start;
                    rr.busyUntil = t.finish;
                }

                totalCost += ResourceAllocator::subsetCost(I, bestSet) * double(t.duration);

                running.push_back({ best, t.finish, bestSet });
                makespan = std::max(makespan, t.finish);
                startedAny = true;

                const double scheduledTaskWeight = demandWeightPerTask[best];
                if (scheduledTaskWeight > 0.0) {
                    for (int ri : demandCapableResIdxPerTask[best]) {
                        futureDemandByRes[ri] -= scheduledTaskWeight;
                        if (futureDemandByRes[ri] < 0.0) futureDemandByRes[ri] = 0.0;
                    }
                }

                cand.erase(std::remove(cand.begin(), cand.end(), best), cand.end());
                scheduled++;
                --unschedCount;

                if (waited) {
                    break;
                }
            }
        }

        if (!startedAny) {
            if (running.empty()) {
                std::cerr << "\n[warn] Brak możliwych startów i brak bieżących zadań. Przerywam.\n";
                break;
            }
            int nextT = std::numeric_limits<int>::max();
            for (auto& rt : running) nextT = std::min(nextT, rt.finish);
            advanceTo(nextT);
        }
    }
    clearFeaturePrecomputed();
    ResourceAllocator::setPrecomputed(nullptr, nullptr, nullptr);
    g_skillLevels = nullptr;
    g_resIndex = nullptr;
    ScheduleResult out;
    out.makespan = makespan;
    out.totalCost = totalCost;
    out.assignedResByImopseTaskIndex = std::move(assignedByImopse);
    return out;
}