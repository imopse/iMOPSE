#include "Scheduler.hpp"
#include <vector>
#include <limits>
#include <algorithm>
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

    struct SchedulerStaticCache {
        std::size_t signature = 0;
        bool ready = false;

        std::vector<int> baseIndeg;
        std::vector<std::vector<int>> succ;

        std::vector<int> feasibleCountPerTask;
        std::vector<double> staticTaskResCount;
        std::vector<double> staticAvgResCost;

        std::vector<std::vector<int>> demandCapableResIdxPerTask;
        std::vector<double> demandWeightPerTask;
        std::vector<double> futureDemandByResInit;
    };

    thread_local ResourceLookupCache g_lookupCache;
    thread_local SchedulerStaticCache g_staticCache;

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

    static std::size_t getTaskStructureSignature(const Instance& I) {
        if (I.taskStructureSignatureReady) {
            return I.taskStructureSignature;
        }

        std::size_t sig = Instance::hashCombine(1099511628211ull, I.tasks.size());

        for (const auto& t : I.tasks) {
            std::size_t taskHash = Instance::hashCombine(std::hash<int>{}(t.id), std::hash<int>{}(t.duration));
            taskHash = Instance::hashCombine(taskHash, std::hash<std::string>{}(t.reqSkill));
            taskHash = Instance::hashCombine(taskHash, std::hash<int>{}(t.reqLevel));
            taskHash = Instance::hashCombine(taskHash, std::hash<int>{}(t.imopseIndex));

            std::size_t predHash = 0;
            for (int pid : t.predecessors) {
                predHash = Instance::hashCombine(predHash, std::hash<int>{}(pid));
            }

            std::size_t capHash = 0;
            for (int rid : t.capableResources) {
                capHash = Instance::hashCombine(capHash, std::hash<int>{}(rid));
            }

            taskHash = Instance::hashCombine(taskHash, predHash);
            taskHash = Instance::hashCombine(taskHash, capHash);

            sig = Instance::hashCombine(sig, taskHash);
        }

        return sig;
    }

    static std::size_t getSchedulerStaticSignature(const Instance& I) {
        std::size_t sig = getResourceStructureSignature(I);
        sig = Instance::hashCombine(sig, getTaskStructureSignature(I));
        sig = Instance::hashCombine(sig, I.tasks.size());
        sig = Instance::hashCombine(sig, I.resources.size());
        return sig;
    }

    static void rebuildSchedulerStaticCache(const Instance& I) {
        const int n = (int)I.tasks.size();
        const auto& __resIndex = g_lookupCache.resIndex;

        g_staticCache.baseIndeg.assign(n, 0);
        g_staticCache.succ.assign(n, {});
        g_staticCache.feasibleCountPerTask.assign(n, 0);
        g_staticCache.staticTaskResCount.assign(n, 0.0);
        g_staticCache.staticAvgResCost.assign(n, std::numeric_limits<double>::infinity());
        g_staticCache.demandCapableResIdxPerTask.assign(n, {});
        g_staticCache.demandWeightPerTask.assign(n, 0.0);
        g_staticCache.futureDemandByResInit.assign(I.resources.size(), 0.0);

        for (int i = 0; i < n; ++i) {
            for (int pid : I.tasks[i].predecessors) {
                if (I.idToIndex.count(pid)) {
                    g_staticCache.baseIndeg[i]++;
                }
            }
        }

        for (int j = 0; j < n; ++j) {
            for (int pid : I.tasks[j].predecessors) {
                auto it = I.idToIndex.find(pid);
                if (it != I.idToIndex.end()) {
                    g_staticCache.succ[it->second].push_back(j);
                }
            }
        }

        for (int ui = 0; ui < n; ++ui) {
            const Task& u = I.tasks[ui];
            const int reqU = std::max(0, u.reqLevel);

            if (reqU <= 0) {
                g_staticCache.feasibleCountPerTask[ui] = (int)I.resources.size();
            }
            else {
                int cnt = 0;
                for (const auto& rr : I.resources) {
                    auto jt = rr.skills.find(u.reqSkill);
                    int ll = (jt != rr.skills.end()) ? jt->second : 0;
                    if (ll >= reqU) ++cnt;
                }
                g_staticCache.feasibleCountPerTask[ui] = cnt;
            }
        }

        for (int ui = 0; ui < n; ++ui) {
            const Task& u = I.tasks[ui];
            const int reqU = std::max(0, u.reqLevel);

            if (reqU <= 0) {
                g_staticCache.staticTaskResCount[ui] = (double)I.resources.size();
                g_staticCache.staticAvgResCost[ui] = std::numeric_limits<double>::infinity();
                continue;
            }

            if (!u.capableResources.empty()) {
                g_staticCache.staticTaskResCount[ui] = (double)u.capableResources.size();

                double sumSal = 0.0;
                int cnt = 0;
                for (int rid : u.capableResources) {
                    auto itIdx = __resIndex.find(rid);
                    if (itIdx == __resIndex.end()) continue;
                    sumSal += I.resources[itIdx->second].salary;
                    ++cnt;
                }
                g_staticCache.staticAvgResCost[ui] =
                    (cnt > 0) ? (sumSal / (double)cnt) : std::numeric_limits<double>::infinity();
                continue;
            }

            g_staticCache.staticTaskResCount[ui] = (double)g_staticCache.feasibleCountPerTask[ui];

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
            g_staticCache.staticAvgResCost[ui] =
                (cnt > 0) ? (sumSal / (double)cnt) : std::numeric_limits<double>::infinity();
        }

        for (int ui = 0; ui < n; ++ui) {
            const Task& u = I.tasks[ui];
            const int reqU = std::max(0, u.reqLevel);
            const double w = 1.0 / (double)std::max(1, g_staticCache.feasibleCountPerTask[ui]);

            g_staticCache.demandWeightPerTask[ui] = w;

            auto& caps = g_staticCache.demandCapableResIdxPerTask[ui];

            if (reqU <= 0) {
                caps.reserve(I.resources.size());
                for (int ri = 0; ri < (int)I.resources.size(); ++ri) {
                    caps.push_back(ri);
                    g_staticCache.futureDemandByResInit[ri] += w;
                }
                continue;
            }

            caps.reserve(std::max(1, g_staticCache.feasibleCountPerTask[ui]));
            for (int ri = 0; ri < (int)I.resources.size(); ++ri) {
                const auto& rr = I.resources[ri];
                auto jt = rr.skills.find(u.reqSkill);
                const int ll = (jt != rr.skills.end()) ? jt->second : 0;
                if (ll >= reqU) {
                    caps.push_back(ri);
                    g_staticCache.futureDemandByResInit[ri] += w;
                }
            }
        }

        g_staticCache.signature = getSchedulerStaticSignature(I);
        g_staticCache.ready = true;
    }

    static void buildSkillStepCache(
        const Instance& I,
        int now,
        std::unordered_map<std::string, SkillStepInfo>& out)
    {
        out.clear();
        out.reserve(g_lookupCache.skillLevels.size());

        for (const auto& kv : g_lookupCache.skillLevels) {
            const std::string& skill = kv.first;
            const std::vector<int>& levels = kv.second;

            int maxLevel = 0;
            for (int lvl : levels) {
                if (lvl > maxLevel) maxLevel = lvl;
            }

            SkillStepInfo info;
            info.maxFreeLevel = 0;

            if (maxLevel <= 0) {
                out.emplace(skill, std::move(info));
                continue;
            }

            const int INF_WAIT = std::numeric_limits<int>::max();
            std::vector<int> exactMinWait(maxLevel + 1, INF_WAIT);
            std::vector<double> exactFirst(maxLevel + 1, std::numeric_limits<double>::infinity());
            std::vector<double> exactSecond(maxLevel + 1, std::numeric_limits<double>::infinity());

            for (int ri = 0; ri < (int)I.resources.size(); ++ri) {
                int lvl = levels[ri];
                if (lvl <= 0) continue;

                const auto& r = I.resources[ri];
                int wait = (r.busyUntil <= now) ? 0 : (r.busyUntil - now);
                if (wait < exactMinWait[lvl]) {
                    exactMinWait[lvl] = wait;
                }

                if (wait == 0) {
                    if (lvl > info.maxFreeLevel) {
                        info.maxFreeLevel = lvl;
                    }

                    double sal = r.salary;
                    if (sal < exactFirst[lvl]) {
                        exactSecond[lvl] = exactFirst[lvl];
                        exactFirst[lvl] = sal;
                    }
                    else if (sal < exactSecond[lvl]) {
                        exactSecond[lvl] = sal;
                    }
                }
            }

            info.minWaitAtLeast.assign(maxLevel + 1, INF_WAIT);
            info.cheapestAtLeast.assign(maxLevel + 1, std::numeric_limits<double>::infinity());
            info.secondCheapestAtLeast.assign(maxLevel + 1, std::numeric_limits<double>::infinity());

            int carryWait = INF_WAIT;
            double carryFirst = std::numeric_limits<double>::infinity();
            double carrySecond = std::numeric_limits<double>::infinity();

            auto feed = [&](double x) {
                if (!std::isfinite(x)) return;
                if (x < carryFirst) {
                    carrySecond = carryFirst;
                    carryFirst = x;
                }
                else if (x < carrySecond) {
                    carrySecond = x;
                }
                };

            for (int lvl = maxLevel; lvl >= 1; --lvl) {
                if (exactMinWait[lvl] < carryWait) {
                    carryWait = exactMinWait[lvl];
                }

                feed(exactFirst[lvl]);
                feed(exactSecond[lvl]);

                info.minWaitAtLeast[lvl] = carryWait;
                info.cheapestAtLeast[lvl] = carryFirst;
                info.secondCheapestAtLeast[lvl] = carrySecond;
            }

            out.emplace(skill, std::move(info));
        }
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

static inline double singleResourceCost(const Instance& I, int resId) {
    const Resource* r = findRes(I, resId);
    return r ? r->salary : 0.0;
}

static inline bool isImopseCapable(const Task& t, int resId) {
    if (t.capableResources.empty()) return true;
    return std::binary_search(t.capableResources.begin(), t.capableResources.end(), resId);
}

static int cheapestSingleCapableNowId(const Instance& I, const Task& t) {
    if (t.capableResources.empty()) return -1;

    int bestId = -1;
    double bestSalary = std::numeric_limits<double>::infinity();

    for (int rid : t.capableResources) {
        const Resource* r = findRes(I, rid);
        if (!r) continue;
        if (r->salary < bestSalary) {
            bestSalary = r->salary;
            bestId = rid;
        }
    }
    return bestId;
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

static inline void insertReadySorted(std::vector<int>& ready, int ix) {
    auto it = std::lower_bound(ready.begin(), ready.end(), ix);
    if (it == ready.end() || *it != ix) {
        ready.insert(it, ix);
    }
}

static inline void eraseReadyValue(std::vector<int>& ready, int ix) {
    auto it = std::lower_bound(ready.begin(), ready.end(), ix);
    if (it != ready.end() && *it == ix) {
        ready.erase(it);
    }
}

static int tryAllocWithForcedId(
    const Instance& I, const Task& t, int forcedResId)
{
    if (forcedResId < 0) return -1;

    if (!isImopseCapable(t, forcedResId)) return -1;
    if (!t.capableResources.empty()) {
        return forcedResId;
    }

    if (t.reqLevel > 0) {
        int lvl = skillLevelOf(I, forcedResId, t.reqSkill);
        if (lvl < t.reqLevel) return -1;
    }
    return forcedResId;
}

static int tryAllocZeroReqWithForcedId(
    const Instance& I, const Task& t, int forcedResId)
{
    return tryAllocWithForcedId(I, t, forcedResId);
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
    const GPTreeResRule* ruleR,
    const ScheduleOptions& options)
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

    const std::size_t staticSig = getSchedulerStaticSignature(I);
    if (!g_staticCache.ready ||
        g_staticCache.signature != staticSig ||
        g_staticCache.baseIndeg.size() != (size_t)n ||
        g_staticCache.futureDemandByResInit.size() != I.resources.size()) {
        rebuildSchedulerStaticCache(I);
    }

    std::vector<int> indeg = g_staticCache.baseIndeg;
    const auto& succ = g_staticCache.succ;
    const auto& feasibleCountPerTask = g_staticCache.feasibleCountPerTask;
    const auto& staticTaskResCount = g_staticCache.staticTaskResCount;
    const auto& staticAvgResCost = g_staticCache.staticAvgResCost;
    const auto& demandCapableResIdxPerTask = g_staticCache.demandCapableResIdxPerTask;
    const auto& demandWeightPerTask = g_staticCache.demandWeightPerTask;
    std::vector<double> futureDemandByRes = g_staticCache.futureDemandByResInit;

    std::vector<int> latestPredFinishByTask(n, 0);

    int unschedCount = n;

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
    if (options.captureAssignedResByImopse) {
        assignedByImopse.assign(n, -1);
    }

    std::vector<int> ready;
    ready.reserve(n);
    std::vector<unsigned char> readyFlag(n, 0);

    for (int i = 0; i < n; ++i) {
        if (indeg[i] == 0 && I.tasks[i].start == -1) {
            ready.push_back(i);
            readyFlag[i] = 1;
        }
    }

    struct Running { int ix; int finish; };
    std::vector<Running> running; running.reserve(n);

    auto processFinishedAtNow = [&]() {
        std::vector<Running> still;
        still.reserve(running.size());

        for (auto& rt : running) {
            if (rt.finish == now) {
                for (int j : succ[rt.ix]) {
                    if (I.tasks[j].start == -1) {
                        indeg[j]--;
                        if (now > latestPredFinishByTask[j]) {
                            latestPredFinishByTask[j] = now;
                        }

                        if (indeg[j] == 0 && !readyFlag[j]) {
                            insertReadySorted(ready, j);
                            readyFlag[j] = 1;
                        }
                    }
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

        bool startedAny = false;

        std::unordered_map<std::string, SkillStepInfo> skillStepCache;
        buildSkillStepCache(I, now, skillStepCache);

        setFeaturePrecomputed(
            &staticTaskResCount,
            &staticAvgResCost,
            &__resIndex,
            &unschedCount,
            &indeg,
            &latestPredFinishByTask,
            &skillStepCache
        );

        while (!ready.empty()) {
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
            int bestResId = -1;
            double bestScore = std::numeric_limits<double>::infinity();
            int minWaitFeasible = std::numeric_limits<int>::max() / 4;

            for (int ix : ready) {
                Task& t = I.tasks[ix];
                const int req = t.reqLevel;

                int forcedId = -1;
                if (g_forcedResource && (size_t)ix < g_forcedResource->size())
                    forcedId = (*g_forcedResource)[ix];

                int allocResId = -1;

                if (forcedId >= 0) {
                    if (req <= 0) {
                        allocResId = tryAllocZeroReqWithForcedId(I, t, forcedId);
                        if (allocResId < 0) {
                            if (auto* r = findRes(I, forcedId)) {
                                int w = std::max(0, r->busyUntil - now);
                                minWaitFeasible = std::min(minWaitFeasible, w);
                            }
                        }
                    }
                    else {
                        allocResId = tryAllocWithForcedId(I, t, forcedId);
                        if (allocResId < 0) {
                            int waitAll = ResourceAllocator::waitUntilFeasible(I, now, t.reqSkill, req);
                            minWaitFeasible = std::min(minWaitFeasible, waitAll);

                            auto fallback = ResourceAllocator::cheapestSubset(I, t.reqSkill, req, now);
                            if (fallback && fallback->size() == 1 && (*fallback)[0] == forcedId) {
                                allocResId = forcedId;
                            }
                        }
                    }
                }
                else {
                    if (ruleR == nullptr) {
                        if (!t.capableResources.empty()) {
                            allocResId = cheapestSingleCapableNowId(I, t);
                            if (allocResId < 0) {
                                int wait = waitUntilAnyCapableFree(I, t, now);
                                minWaitFeasible = std::min(minWaitFeasible, wait);
                            }
                        }
                        else {
                            auto pick = ResourceAllocator::cheapestSubset(I, t.reqSkill, req, now);
                            if (pick && pick->size() == 1) {
                                allocResId = (*pick)[0];
                            }
                            else {
                                int wait = ResourceAllocator::waitUntilFeasible(I, now, t.reqSkill, req);
                                minWaitFeasible = std::min(minWaitFeasible, wait);
                            }
                        }
                    }
                    else {
                        double cheapestNow = std::numeric_limits<double>::infinity();
                        {
                            const int req0 = std::max(0, t.reqLevel);

                            if (req0 > 0 && !t.reqSkill.empty()) {
                                auto itSkill = skillStepCache.find(t.reqSkill);
                                if (itSkill != skillStepCache.end()) {
                                    const auto& info = itSkill->second;
                                    if (req0 < (int)info.cheapestAtLeast.size()) {
                                        cheapestNow = info.cheapestAtLeast[req0];
                                    }
                                }
                            }

                            if (!std::isfinite(cheapestNow)) {
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
                        }

                        int localBestResId = -1;
                        double localBestResScore = std::numeric_limits<double>::infinity();

                        if (!t.capableResources.empty()) {
                            for (int rid : t.capableResources) {
                                auto it = __resIndex.find(rid);
                                if (it == __resIndex.end()) continue;

                                const Resource& r = I.resources[it->second];
                                double futureDemandExcludingTask = futureDemandByRes[it->second] - demandWeightPerTask[ix];
                                if (futureDemandExcludingTask < 0.0) futureDemandExcludingTask = 0.0;

                                double s = ruleR->scoreFast(I, ix, t, r, now, cheapestNow, futureDemandExcludingTask);
                                if (s < localBestResScore) {
                                    localBestResScore = s;
                                    localBestResId = r.id;
                                }
                            }
                        }
                        else {
                            for (const auto& r : I.resources) {
                                if (req > 0) {
                                    int lvl = skillLevelOf(I, r.id, t.reqSkill);
                                    if (lvl < req) continue;
                                }

                                const int ri = (int)(&r - &I.resources[0]);
                                double futureDemandExcludingTask = futureDemandByRes[ri] - demandWeightPerTask[ix];
                                if (futureDemandExcludingTask < 0.0) futureDemandExcludingTask = 0.0;

                                double s = ruleR->scoreFast(I, ix, t, r, now, cheapestNow, futureDemandExcludingTask);
                                if (s < localBestResScore) {
                                    localBestResScore = s;
                                    localBestResId = r.id;
                                }
                            }
                        }

                        if (localBestResId < 0) {
                            int wait = (!t.capableResources.empty())
                                ? waitUntilAnyCapableFree(I, t, now)
                                : ResourceAllocator::waitUntilFeasible(I, now, t.reqSkill, req);

                            minWaitFeasible = std::min(minWaitFeasible, wait);
                        }
                        else {
                            allocResId = localBestResId;
                        }
                    }
                }


                if (allocResId < 0) {
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
                    ScoreTrace tr = gp->scoreWithTraceFast(ix, t);
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
                    if (const auto* gpFast = dynamic_cast<const GPTreeRule*>(&ruleT)) {
                        sc = gpFast->scoreFast(ix, t);
                    }
                    else {
                        sc = ruleT.score(t);
                    }
                }

                if (sc < bestScore) {
                    bestScore = sc;
                    best = ix;
                    bestResId = allocResId;
                }
                else if (best != -1 && std::abs(sc - bestScore) < 1e-9 && g_priorityKeys) {
                    const auto& K = *g_priorityKeys;
                    if ((size_t)best < K.size() && (size_t)ix < K.size()) {
                        if (K[ix] < K[best]) {
                            best = ix;
                            bestResId = allocResId;
                        }
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
                {
                    auto it = __resIndex.find(bestResId);
                    if (it != __resIndex.end()) {
                        desiredStart = std::max(desiredStart, I.resources[it->second].busyUntil);
                    }
                }

                bool waited = false;
                if (desiredStart > now) {
                    advanceTo(desiredStart);
                    waited = true;
                }

                t.start = now;
                t.finish = now + t.duration;

                if (options.keepTaskAssignedResources) {
                    t.assignedResources.clear();
                    if (bestResId >= 0) {
                        t.assignedResources.push_back(bestResId);
                    }
                }

                if (options.captureAssignedResByImopse &&
                    bestResId >= 0 &&
                    t.imopseIndex >= 0 &&
                    t.imopseIndex < (int)assignedByImopse.size()) {
                    assignedByImopse[t.imopseIndex] = bestResId;
                }

                if (bestResId >= 0) {
                    auto it = __resIndex.find(bestResId);
                    if (it != __resIndex.end()) {
                        auto& rr = I.resources[it->second];
                        rr.busy = true;
                        rr.busyStart = t.start;
                        rr.busyUntil = t.finish;
                    }
                }

                if (options.computeObjectiveStats && bestResId >= 0) {
                    totalCost += singleResourceCost(I, bestResId) * double(t.duration);
                }

                running.push_back({ best, t.finish });
                if (options.computeObjectiveStats) {
                    makespan = std::max(makespan, t.finish);
                }
                startedAny = true;

                const double scheduledTaskWeight = demandWeightPerTask[best];
                if (scheduledTaskWeight > 0.0) {
                    for (int ri : demandCapableResIdxPerTask[best]) {
                        futureDemandByRes[ri] -= scheduledTaskWeight;
                        if (futureDemandByRes[ri] < 0.0) futureDemandByRes[ri] = 0.0;
                    }
                }

                eraseReadyValue(ready, best);
                readyFlag[best] = 0;
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
    out.makespan = options.computeObjectiveStats ? makespan : 0;
    out.totalCost = options.computeObjectiveStats ? totalCost : 0.0;
    if (options.captureAssignedResByImopse) {
        out.assignedResByImopseTaskIndex = std::move(assignedByImopse);
    }
    return out;
}