#include "Scheduler.hpp"
#include <vector>
#include <limits>
#include <algorithm>
#include <cmath>
#include <string>
#include "../alloc/ResourceAllocator.hpp"
#include "../rules/GPTreeRule.hpp"
#include "../rules/GPTreeResRule.hpp"
#include "../gp/FeatureScaling.hpp"
#include "../gp/Precompute.hpp"
#include "../gp/Features.hpp"
#include <unordered_map>

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
        std::vector<int> matchedLevelByTaskRes;
        std::vector<double> reservePressureWeightPerTask;
        std::vector<double> criticalReserveWeightPerTask;
        std::vector<int> familyIdByTask;
        int familyCount = 0;
        std::vector<double> reservePressureByResInit;
        std::vector<double> criticalReserveByResInit;
        std::vector<double> familyPressureByResFamilyInit;
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
            taskHash = Instance::hashCombine(taskHash, std::hash<std::string>{}(t.requirementKey()));
            taskHash = Instance::hashCombine(taskHash, std::hash<int>{}(t.totalRequiredLevel()));
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
        g_staticCache.matchedLevelByTaskRes.assign((size_t)n * I.resources.size(), 0);
        g_staticCache.reservePressureWeightPerTask.assign(n, 0.0);
        g_staticCache.criticalReserveWeightPerTask.assign(n, 0.0);
        g_staticCache.familyIdByTask.assign(n, -1);
        g_staticCache.familyCount = 0;
        g_staticCache.reservePressureByResInit.assign(I.resources.size(), 0.0);
        g_staticCache.criticalReserveByResInit.assign(I.resources.size(), 0.0);
        g_staticCache.familyPressureByResFamilyInit.clear();


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
        {
            const int resCount = (int)I.resources.size();

            for (int ti = 0; ti < n; ++ti) {
                const Task& t = I.tasks[ti];
                const size_t base = (size_t)ti * (size_t)resCount;

                for (int ri = 0; ri < resCount; ++ri) {
                    const Resource& r = I.resources[ri];
                    int matched = 0;

                    if (t.requiredSkills.empty()) {
                        auto it = r.skills.find(t.reqSkill);
                        if (it != r.skills.end()) {
                            matched = it->second;
                        }
                    }
                    else {
                        for (const auto& rs : t.requiredSkills) {
                            auto it = r.skills.find(rs.skill);
                            if (it != r.skills.end()) {
                                matched += it->second;
                            }
                        }
                    }

                    g_staticCache.matchedLevelByTaskRes[base + (size_t)ri] = matched;
                }
            }
        }
        for (int ui = 0; ui < n; ++ui) {
            const Task& u = I.tasks[ui];
            const int reqU = std::max(0, u.totalRequiredLevel());

            if (reqU <= 0) {
                g_staticCache.feasibleCountPerTask[ui] = (int)I.resources.size();
                g_staticCache.staticTaskResCount[ui] = (double)I.resources.size();
                g_staticCache.staticAvgResCost[ui] = std::numeric_limits<double>::infinity();
                continue;
            }

            if (!u.capableResourceIndices.empty()) {
                g_staticCache.feasibleCountPerTask[ui] = (int)u.capableResourceIndices.size();
                g_staticCache.staticTaskResCount[ui] = (double)u.capableResourceIndices.size();

                double sumSal = 0.0;
                int cnt = 0;
                for (int ri : u.capableResourceIndices) {
                    if (ri < 0 || ri >= (int)I.resources.size()) continue;
                    sumSal += I.resources[ri].salary;
                    ++cnt;
                }

                g_staticCache.staticAvgResCost[ui] =
                    (cnt > 0) ? (sumSal / (double)cnt) : std::numeric_limits<double>::infinity();
                continue;
            }

            int feasibleCnt = 0;
            double sumSal = 0.0;
            int cnt = 0;

            for (const auto& rr : I.resources) {
                if (!u.canBeDoneBy(rr)) continue;
                ++feasibleCnt;
                sumSal += rr.salary;
                ++cnt;
            }

            g_staticCache.feasibleCountPerTask[ui] = feasibleCnt;
            g_staticCache.staticTaskResCount[ui] = (double)feasibleCnt;
            g_staticCache.staticAvgResCost[ui] =
                (cnt > 0) ? (sumSal / (double)cnt) : std::numeric_limits<double>::infinity();
        }

        {
            std::unordered_map<std::string, int> familyIndex;
            familyIndex.reserve((size_t)n * 2);

            for (int ui = 0; ui < n; ++ui) {
                const Task& u = I.tasks[ui];
                const std::string key = u.requirementKey();

                auto it = familyIndex.find(key);
                if (it == familyIndex.end()) {
                    const int id = (int)familyIndex.size();
                    familyIndex.emplace(key, id);
                    g_staticCache.familyIdByTask[ui] = id;
                }
                else {
                    g_staticCache.familyIdByTask[ui] = it->second;
                }
            }

            g_staticCache.familyCount = std::max(1, (int)familyIndex.size());
            g_staticCache.familyPressureByResFamilyInit.assign(
                I.resources.size() * (size_t)g_staticCache.familyCount,
                0.0
            );
        }

        for (int ui = 0; ui < n; ++ui) {
            const Task& u = I.tasks[ui];
            const int reqU = std::max(0, u.totalRequiredLevel());

            const int feasibleCount = std::max(1, g_staticCache.feasibleCountPerTask[ui]);

            double cheapest = std::numeric_limits<double>::infinity();
            double second = std::numeric_limits<double>::infinity();

            if (!u.capableResourceIndices.empty()) {
                for (int ri : u.capableResourceIndices) {
                    if (ri < 0 || ri >= (int)I.resources.size()) continue;
                    const double sal = I.resources[ri].salary;

                    if (sal < cheapest) {
                        second = cheapest;
                        cheapest = sal;
                    }
                    else if (sal < second) {
                        second = sal;
                    }
                }
            }
            else {
                for (int ri = 0; ri < (int)I.resources.size(); ++ri) {
                    const auto& rr = I.resources[ri];
                    if (reqU > 0 && !u.canBeDoneBy(rr)) continue;

                    const double sal = rr.salary;
                    if (sal < cheapest) {
                        second = cheapest;
                        cheapest = sal;
                    }
                    else if (sal < second) {
                        second = sal;
                    }
                }
            }

            if (!std::isfinite(second)) second = cheapest;
            const double priceGap = std::max(0.0, second - cheapest);
            const double reserveW = ((double)u.duration * priceGap) / (double)feasibleCount;

            const auto& S = gp::getFeatureScaling();

            double critRaw = 0.0;
            double slackRaw = 0.0;
            double descRaw = 0.0;

            if (auto cpm = gp::getCPMPrecalc()) {
                if (ui >= 0 && ui < (int)cpm->critLen.size())   critRaw = cpm->critLen[ui];
                if (ui >= 0 && ui < (int)cpm->slack.size())     slackRaw = cpm->slack[ui];
                if (ui >= 0 && ui < (int)cpm->descCount.size()) descRaw = cpm->descCount[ui];
            }

            const double critNorm =
                (S.maxCritLen > 0.0) ? (critRaw / S.maxCritLen) : 0.0;
            const double slackNorm =
                (S.maxSlackPos > 0.0) ? (std::max(0.0, slackRaw) / S.maxSlackPos) : 0.0;
            const double descNorm =
                (S.maxNumTasks > 0.0) ? (descRaw / S.maxNumTasks) : 0.0;

            const double structuralPressure =
                (1.0 + critNorm + descNorm) / (1.0 + slackNorm);

            const double criticalReserveW = reserveW * structuralPressure;

            const int famId = g_staticCache.familyIdByTask[ui];

            g_staticCache.reservePressureWeightPerTask[ui] = reserveW;
            g_staticCache.criticalReserveWeightPerTask[ui] = criticalReserveW;

            const double scarcityFuture = (reqU > 0)
                ? ((double)reqU / (double)std::max(1, feasibleCount))
                : 0.0;

            const double structuralFuture =
                (1.0 + critNorm + 0.5 * descNorm) / (1.0 + slackNorm);

            const double futureBase = (double)u.duration * scarcityFuture * structuralFuture;

            auto& caps = g_staticCache.demandCapableResIdxPerTask[ui];

            if (reqU <= 0) {
                caps.reserve(I.resources.size());
                for (int ri = 0; ri < (int)I.resources.size(); ++ri) {
                    caps.push_back(ri);
                    g_staticCache.reservePressureByResInit[ri] += reserveW;
                    g_staticCache.criticalReserveByResInit[ri] += criticalReserveW;
                    g_staticCache.familyPressureByResFamilyInit[
                        (size_t)ri * (size_t)g_staticCache.familyCount + (size_t)famId
                    ] += reserveW;
                }
                continue;
            }

            caps.reserve(std::max(1, feasibleCount));

            if (!u.capableResourceIndices.empty()) {
                for (int ri : u.capableResourceIndices) {
                    if (ri < 0 || ri >= (int)I.resources.size()) continue;

                    caps.push_back(ri);
                    g_staticCache.reservePressureByResInit[ri] += reserveW;
                    g_staticCache.criticalReserveByResInit[ri] += criticalReserveW;
                    g_staticCache.familyPressureByResFamilyInit[
                        (size_t)ri * (size_t)g_staticCache.familyCount + (size_t)famId
                    ] += reserveW;
                }
            }
            else {
                for (int ri = 0; ri < (int)I.resources.size(); ++ri) {
                    const auto& rr = I.resources[ri];
                    if (reqU > 0 && !u.canBeDoneBy(rr)) continue;

                    caps.push_back(ri);
                    g_staticCache.reservePressureByResInit[ri] += reserveW;
                    g_staticCache.criticalReserveByResInit[ri] += criticalReserveW;
                    g_staticCache.familyPressureByResFamilyInit[
                        (size_t)ri * (size_t)g_staticCache.familyCount + (size_t)famId
                    ] += reserveW;
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
    if (t.capableResourceIndices.empty()) return -1;

    int bestId = -1;
    double bestSalary = std::numeric_limits<double>::infinity();

    for (int ri : t.capableResourceIndices) {
        if (ri < 0 || ri >= (int)I.resources.size()) continue;
        const auto& r = I.resources[ri];
        if (r.salary < bestSalary) {
            bestSalary = r.salary;
            bestId = r.id;
        }
    }
    return bestId;
}

static int waitUntilAnyCapableFree(const Instance& I, const Task& t, int now) {
    if (t.capableResourceIndices.empty()) return std::numeric_limits<int>::max() / 4;

    int best = std::numeric_limits<int>::max() / 4;
    for (int ri : t.capableResourceIndices) {
        if (ri < 0 || ri >= (int)I.resources.size()) continue;
        const auto& r = I.resources[ri];
        int w = std::max(0, r.busyUntil - now);
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

ScheduleResult Scheduler::withResources(Instance& I,
    const IDispatchingRule& ruleT,
    const GPTreeResRule* ruleR,
    const ScheduleOptions& options,
    const GPTree* pairTree)
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
        g_staticCache.reservePressureByResInit.size() != I.resources.size() ||
        g_staticCache.matchedLevelByTaskRes.size() !=
        (size_t)n * I.resources.size() ||
        g_staticCache.familyPressureByResFamilyInit.size() !=
        I.resources.size() * (size_t)std::max(1, g_staticCache.familyCount)) {
        rebuildSchedulerStaticCache(I);
    }

    std::vector<int> indeg = g_staticCache.baseIndeg;
    const auto& succ = g_staticCache.succ;
    const auto& feasibleCountPerTask = g_staticCache.feasibleCountPerTask;
    const auto& staticTaskResCount = g_staticCache.staticTaskResCount;
    const auto& staticAvgResCost = g_staticCache.staticAvgResCost;
    const auto& demandCapableResIdxPerTask = g_staticCache.demandCapableResIdxPerTask;
    const auto& reservePressureWeightPerTask = g_staticCache.reservePressureWeightPerTask;
    const auto& criticalReserveWeightPerTask = g_staticCache.criticalReserveWeightPerTask;
    const auto& familyIdByTask = g_staticCache.familyIdByTask;
    const int familyCount = g_staticCache.familyCount;
    std::vector<double> reservePressureByRes = g_staticCache.reservePressureByResInit;
    std::vector<double> criticalReserveByRes = g_staticCache.criticalReserveByResInit;
    std::vector<double> familyPressureByResFamily = g_staticCache.familyPressureByResFamilyInit;

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
        size_t write = 0;

        for (size_t i = 0; i < running.size(); ++i) {
            const auto& rt = running[i];

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
                if (write != i) {
                    running[write] = running[i];
                }
                ++write;
            }
        }

        running.resize(write);
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
    const GPTreeRule* gpRule = dynamic_cast<const GPTreeRule*>(&ruleT);
    const bool usePairTree = (pairTree != nullptr);

    auto treeContainsFeature = [](const GPTree* tree, FeatureId feat) -> bool {
        if (!tree) return false;

        for (const auto& n : tree->nodes) {
            if (n.kind == NodeKind::FEATURE && n.feat == feat) {
                return true;
            }
        }

        return false;
        };

    const GPTree* activeTaskTree = usePairTree
        ? pairTree
        : (gpRule ? &gpRule->getTree() : nullptr);

    const bool needTaskCritLen =
        treeContainsFeature(activeTaskTree, FeatureId::CRITLEN);
    const bool needTaskSlack =
        treeContainsFeature(activeTaskTree, FeatureId::SLACK);
    const bool needTaskDescCount =
        treeContainsFeature(activeTaskTree, FeatureId::DESC_COUNT);
    const bool needTaskReleasePressure =
        treeContainsFeature(activeTaskTree, FeatureId::TASK_RELEASE_PRESSURE);
    const bool needTaskCriticalPressure =
        treeContainsFeature(activeTaskTree, FeatureId::TASK_CRITICAL_PRESSURE);
    const bool needTaskAvailSkill =
        treeContainsFeature(activeTaskTree, FeatureId::AVAIL_SKILL);
    const bool needTaskAvailGap =
        treeContainsFeature(activeTaskTree, FeatureId::AVAIL_GAP);
    const bool needTaskCheapestCostNow =
        treeContainsFeature(activeTaskTree, FeatureId::CHEAPEST_COST_NOW);
    const bool needTaskCostPerSkillNow =
        treeContainsFeature(activeTaskTree, FeatureId::COST_PER_SKILL_NOW);
    const bool needTaskMinFeasibleCostNow =
        treeContainsFeature(activeTaskTree, FeatureId::MIN_FEASIBLE_COST_NOW);
    const bool needTaskCostRegretNow =
        treeContainsFeature(activeTaskTree, FeatureId::COST_REGRET_NOW);
    const bool needTaskResCount =
        treeContainsFeature(activeTaskTree, FeatureId::TASK_RES_COUNT);
    const bool needTaskAvgResCost =
        treeContainsFeature(activeTaskTree, FeatureId::AVG_RES_COST);
    const bool needTaskUnschedTasks =
        treeContainsFeature(activeTaskTree, FeatureId::UNSCHED_TASKS);

    const bool needTaskStructureFeatures =
        needTaskCritLen ||
        needTaskSlack ||
        needTaskDescCount ||
        needTaskReleasePressure ||
        needTaskCriticalPressure;

    const bool needTaskAvailabilityFeatures =
        needTaskAvailSkill ||
        needTaskAvailGap ||
        needTaskCheapestCostNow ||
        needTaskCostPerSkillNow ||
        needTaskMinFeasibleCostNow ||
        needTaskCostRegretNow;

    const bool needTaskCostNowFeatures =
        needTaskCheapestCostNow ||
        needTaskCostPerSkillNow ||
        needTaskMinFeasibleCostNow ||
        needTaskCostRegretNow;

    setOptionalTaskFeatureUsage(
        needTaskStructureFeatures,
        needTaskResCount,
        needTaskAvgResCost,
        needTaskUnschedTasks,
        needTaskAvailabilityFeatures,
        needTaskCostNowFeatures,
        needTaskReleasePressure,
        needTaskCriticalPressure
    );

    const GPTree* activeResourceTree = usePairTree
        ? pairTree
        : (ruleR ? ruleR->tree : nullptr);

    const bool needResReservePressure =
        treeContainsFeature(activeResourceTree, FeatureId::RES_RESERVE_PRESSURE);
    const bool needResFamilyMismatch =
        treeContainsFeature(activeResourceTree, FeatureId::RES_FAMILY_MISMATCH);
    const bool needResFutureBranchFit =
        treeContainsFeature(activeResourceTree, FeatureId::RES_FUTURE_BRANCH_FIT);
    const bool needResBottleneckPreservation =
        treeContainsFeature(activeResourceTree, FeatureId::RES_BOTTLENECK_PRESERVATION);
    const bool needResSpecialistMisuse =
        treeContainsFeature(activeResourceTree, FeatureId::RES_SPECIALIST_MISUSE);

    setOptionalResourceFeatureUsage(
        needResFutureBranchFit,
        needResBottleneckPreservation,
        needResSpecialistMisuse
    );

    std::unordered_map<std::string, SkillStepInfo> skillStepCache;
    if (needTaskAvailabilityFeatures || needTaskCostNowFeatures) {
        skillStepCache.reserve(g_lookupCache.skillLevels.size());
    }

    while (scheduled < n) {
        freeResources(now);

        bool startedAny = false;

        if (needTaskAvailabilityFeatures || needTaskCostNowFeatures) {
            buildSkillStepCache(I, now, skillStepCache);
        }
        else {
            skillStepCache.clear();
        }

        setFeaturePrecomputed(
            &staticTaskResCount,
            &staticAvgResCost,
            &__resIndex,
            &unschedCount,
            &indeg,
            &latestPredFinishByTask,
            &skillStepCache,
            &g_staticCache.matchedLevelByTaskRes,
            (int)I.resources.size()
        );

        while (!ready.empty()) {
            const_cast<IDispatchingRule&>(ruleT).setContext(&I, now);

            buildTaskEvalStepPrecomputed(
                I,
                now,
                ready
            );

            if (usePairTree) {
                buildPairEvalStepPrecomputed(
                    I,
                    now,
                    ready
                );
            }
            else {
                clearPairEvalStepPrecomputed();
            }

            constexpr double kPairResourceWeight = 0.5;

            int best = -1;
            int bestResId = -1;
            double bestScore = std::numeric_limits<double>::infinity();
            int minWaitFeasible = std::numeric_limits<int>::max() / 4;

            struct TaskCandidate {
                int ix = -1;
                int allocResId = -1;
                double taskScore = std::numeric_limits<double>::infinity();
                double resScore = std::numeric_limits<double>::infinity();
                double pairScore = std::numeric_limits<double>::infinity();
            };

            auto norm01 = [](double v, double lo, double hi) -> double {
                if (!std::isfinite(v)) return 1.0;
                if (!std::isfinite(lo) || !std::isfinite(hi)) return 0.0;
                if (hi <= lo + 1e-12) return 0.0;
                double x = (v - lo) / (hi - lo);
                if (x < 0.0) x = 0.0;
                if (x > 1.0) x = 1.0;
                return x;
                };

            std::vector<TaskCandidate> pairCandidates;
            if (!usePairTree) {
                pairCandidates.reserve(ready.size());
            }

            std::vector<double> familyBestPressureByRes;
            std::vector<double> familySecondBestPressureByRes;
            std::vector<int> familyBestFamByRes;

            if (familyCount > 0 && needResFamilyMismatch) {
                const int resCount = (int)I.resources.size();

                familyBestPressureByRes.assign(resCount, 0.0);
                familySecondBestPressureByRes.assign(resCount, 0.0);
                familyBestFamByRes.assign(resCount, -1);

                for (int ri = 0; ri < resCount; ++ri) {
                    const size_t base = (size_t)ri * (size_t)familyCount;

                    double best1 = 0.0;
                    double best2 = 0.0;
                    int bestFam = -1;

                    for (int f = 0; f < familyCount; ++f) {
                        const double v = familyPressureByResFamily[base + (size_t)f];

                        if (v > best1) {
                            best2 = best1;
                            best1 = v;
                            bestFam = f;
                        }
                        else if (v > best2) {
                            best2 = v;
                        }
                    }

                    familyBestPressureByRes[ri] = best1;
                    familySecondBestPressureByRes[ri] = best2;
                    familyBestFamByRes[ri] = bestFam;
                }
            }

            for (int ix : ready) {
                Task& t = I.tasks[ix];
                const int req = t.reqLevel;

                int localBestResId = -1;
                double localBestResScore = 0.0;
                double localBestPairScore = std::numeric_limits<double>::infinity();

                const int forcedId = -1;

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
                        else {
                            localBestResId = allocResId;
                            localBestResScore = 0.0;
                            localBestPairScore = 0.0;
                        }
                    }
                    else {
                        allocResId = tryAllocWithForcedId(I, t, forcedId);
                        if (allocResId < 0) {
                            int waitAll = ResourceAllocator::waitUntilFeasible(I, now, t.reqSkill, req);
                            minWaitFeasible = std::min(minWaitFeasible, waitAll);

                            int fallbackId = ResourceAllocator::cheapestSubsetSingleId(I, t.reqSkill, req, now);
                            if (fallbackId == forcedId) {
                                allocResId = forcedId;
                            }
                        }
                        if (allocResId >= 0) {
                            localBestResId = allocResId;
                            localBestResScore = 0.0;
                            localBestPairScore = 0.0;
                        }
                    }
                }
                else {
                    if (usePairTree) {
                        double cheapestNow = std::numeric_limits<double>::infinity();
                        double cheapestCapableOverall = std::numeric_limits<double>::infinity();
                        double waitOfCheapestCapableOverall = std::numeric_limits<double>::infinity();

                        {
                            const int req0 = std::max(0, t.reqLevel);

                            if (!t.capableResourceIndices.empty()) {
                                for (int ri : t.capableResourceIndices) {
                                    if (ri < 0 || ri >= (int)I.resources.size()) continue;

                                    const auto& rr = I.resources[ri];
                                    const double waitRR =
                                        (rr.busyUntil > now) ? double(rr.busyUntil - now) : 0.0;

                                    if (rr.salary < cheapestCapableOverall) {
                                        cheapestCapableOverall = rr.salary;
                                        waitOfCheapestCapableOverall = waitRR;
                                    }
                                    else if (rr.salary == cheapestCapableOverall &&
                                        waitRR < waitOfCheapestCapableOverall) {
                                        waitOfCheapestCapableOverall = waitRR;
                                    }

                                    if (rr.busyUntil <= now) {
                                        cheapestNow = std::min(cheapestNow, rr.salary);
                                    }
                                }
                            }
                            else {
                                for (const auto& rr : I.resources) {
                                    int lvl = 0;
                                    auto it = rr.skills.find(t.reqSkill);
                                    if (it != rr.skills.end()) lvl = it->second;

                                    if (req0 > 0 && lvl < req0) continue;

                                    const double waitRR =
                                        (rr.busyUntil > now) ? double(rr.busyUntil - now) : 0.0;

                                    if (rr.salary < cheapestCapableOverall) {
                                        cheapestCapableOverall = rr.salary;
                                        waitOfCheapestCapableOverall = waitRR;
                                    }
                                    else if (rr.salary == cheapestCapableOverall &&
                                        waitRR < waitOfCheapestCapableOverall) {
                                        waitOfCheapestCapableOverall = waitRR;
                                    }

                                    if (rr.busyUntil <= now) {
                                        cheapestNow = std::min(cheapestNow, rr.salary);
                                    }
                                }
                            }
                        }

                        localBestResId = -1;
                        localBestPairScore = std::numeric_limits<double>::infinity();

                        if (!t.capableResourceIndices.empty()) {
                            for (int ri : t.capableResourceIndices) {
                                if (ri < 0 || ri >= (int)I.resources.size()) continue;

                                const Resource& r = I.resources[ri];

                                double reservePressureExcludingTask = 0.0;
                                if (needResReservePressure) {
                                    reservePressureExcludingTask =
                                        reservePressureByRes[ri] - reservePressureWeightPerTask[ix];
                                    if (reservePressureExcludingTask < 0.0)
                                        reservePressureExcludingTask = 0.0;
                                }

                                double criticalReserveExcludingTask = 0.0;
                                if (needResBottleneckPreservation || needResSpecialistMisuse) {
                                    criticalReserveExcludingTask =
                                        criticalReserveByRes[ri] - criticalReserveWeightPerTask[ix];
                                    if (criticalReserveExcludingTask < 0.0)
                                        criticalReserveExcludingTask = 0.0;
                                }

                                double familyMismatchExcludingTask = 0.0;
                                if (needResFamilyMismatch && familyCount > 0) {
                                    const int famId = familyIdByTask[ix];
                                    const size_t base = (size_t)ri * (size_t)familyCount;

                                    double currentFamilyPressure =
                                        familyPressureByResFamily[base + (size_t)famId]
                                        - reservePressureWeightPerTask[ix];
                                    if (currentFamilyPressure < 0.0)
                                        currentFamilyPressure = 0.0;

                                    const double bestOtherFamilyPressure =
                                        (familyBestFamByRes[ri] == famId)
                                        ? familySecondBestPressureByRes[ri]
                                        : familyBestPressureByRes[ri];

                                        familyMismatchExcludingTask =
                                            bestOtherFamilyPressure / (1.0 + currentFamilyPressure);
                                }

                                    Features f = computePairFeaturesFast(
                                        I,
                                        ix,
                                        t,
                                        r,
                                        now,
                                        cheapestNow,
                                        cheapestCapableOverall,
                                        waitOfCheapestCapableOverall,
                                        reservePressureExcludingTask,
                                        criticalReserveExcludingTask,
                                        familyMismatchExcludingTask
                                    );

                                    double s = pairTree->eval(f);

                                    if (s < localBestPairScore) {
                                        localBestPairScore = s;
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

                                double reservePressureExcludingTask = 0.0;
                                if (needResReservePressure) {
                                    reservePressureExcludingTask =
                                        reservePressureByRes[ri] - reservePressureWeightPerTask[ix];
                                    if (reservePressureExcludingTask < 0.0)
                                        reservePressureExcludingTask = 0.0;
                                }

                                double criticalReserveExcludingTask = 0.0;
                                if (needResBottleneckPreservation || needResSpecialistMisuse) {
                                    criticalReserveExcludingTask =
                                        criticalReserveByRes[ri] - criticalReserveWeightPerTask[ix];
                                    if (criticalReserveExcludingTask < 0.0)
                                        criticalReserveExcludingTask = 0.0;
                                }

                                double familyMismatchExcludingTask = 0.0;
                                if (needResFamilyMismatch && familyCount > 0) {
                                    const int famId = familyIdByTask[ix];
                                    const size_t base = (size_t)ri * (size_t)familyCount;

                                    double currentFamilyPressure =
                                        familyPressureByResFamily[base + (size_t)famId]
                                        - reservePressureWeightPerTask[ix];
                                    if (currentFamilyPressure < 0.0)
                                        currentFamilyPressure = 0.0;

                                    const double bestOtherFamilyPressure =
                                        (familyBestFamByRes[ri] == famId)
                                        ? familySecondBestPressureByRes[ri]
                                        : familyBestPressureByRes[ri];

                                        familyMismatchExcludingTask =
                                            bestOtherFamilyPressure / (1.0 + currentFamilyPressure);
                                }

                                    Features f = computePairFeaturesFast(
                                        I,
                                        ix,
                                        t,
                                        r,
                                        now,
                                        cheapestNow,
                                        cheapestCapableOverall,
                                        waitOfCheapestCapableOverall,
                                        reservePressureExcludingTask,
                                        criticalReserveExcludingTask,
                                        familyMismatchExcludingTask
                                    );

                                    double s = pairTree->eval(f);

                                    if (s < localBestPairScore) {
                                        localBestPairScore = s;
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
                    else if (ruleR == nullptr) {
                        if (!t.capableResources.empty()) {
                            allocResId = cheapestSingleCapableNowId(I, t);
                            if (allocResId < 0) {
                                int wait = waitUntilAnyCapableFree(I, t, now);
                                minWaitFeasible = std::min(minWaitFeasible, wait);
                            }
                            else {
                                localBestResId = allocResId;
                                localBestResScore = 0.0;
                            }
                        }
                        else {
                            int pickId = ResourceAllocator::cheapestSubsetSingleId(I, t.reqSkill, req, now);
                            if (pickId >= 0) {
                                allocResId = pickId;
                                localBestResId = allocResId;
                                localBestResScore = 0.0;
                            }
                            else {
                                int wait = ResourceAllocator::waitUntilFeasible(I, now, t.reqSkill, req);
                                minWaitFeasible = std::min(minWaitFeasible, wait);
                            }
                        }
                    }
                    else {
                        double cheapestNow = std::numeric_limits<double>::infinity();
                        double cheapestCapableOverall = std::numeric_limits<double>::infinity();
                        double waitOfCheapestCapableOverall = std::numeric_limits<double>::infinity();

                        {
                            const int req0 = std::max(0, t.reqLevel);

                            if (!t.capableResourceIndices.empty()) {
                                for (int ri : t.capableResourceIndices) {
                                    if (ri < 0 || ri >= (int)I.resources.size()) continue;

                                    const auto& rr = I.resources[ri];
                                    const double waitRR =
                                        (rr.busyUntil > now) ? double(rr.busyUntil - now) : 0.0;

                                    if (rr.salary < cheapestCapableOverall) {
                                        cheapestCapableOverall = rr.salary;
                                        waitOfCheapestCapableOverall = waitRR;
                                    }
                                    else if (rr.salary == cheapestCapableOverall &&
                                        waitRR < waitOfCheapestCapableOverall) {
                                        waitOfCheapestCapableOverall = waitRR;
                                    }

                                    if (rr.busyUntil <= now) {
                                        cheapestNow = std::min(cheapestNow, rr.salary);
                                    }
                                }
                            }
                            else {
                                for (const auto& rr : I.resources) {
                                    int lvl = 0;
                                    auto it = rr.skills.find(t.reqSkill);
                                    if (it != rr.skills.end()) lvl = it->second;

                                    if (req0 > 0 && lvl < req0) continue;

                                    const double waitRR =
                                        (rr.busyUntil > now) ? double(rr.busyUntil - now) : 0.0;

                                    if (rr.salary < cheapestCapableOverall) {
                                        cheapestCapableOverall = rr.salary;
                                        waitOfCheapestCapableOverall = waitRR;
                                    }
                                    else if (rr.salary == cheapestCapableOverall &&
                                        waitRR < waitOfCheapestCapableOverall) {
                                        waitOfCheapestCapableOverall = waitRR;
                                    }

                                    if (rr.busyUntil <= now) {
                                        cheapestNow = std::min(cheapestNow, rr.salary);
                                    }
                                }
                            }
                        }

                        localBestResId = -1;
                        localBestResScore = std::numeric_limits<double>::infinity();

                        if (!t.capableResourceIndices.empty()) {
                            for (int ri : t.capableResourceIndices) {
                                if (ri < 0 || ri >= (int)I.resources.size()) continue;

                                const Resource& r = I.resources[ri];

                                double reservePressureExcludingTask = 0.0;
                                if (needResReservePressure) {
                                    reservePressureExcludingTask =
                                        reservePressureByRes[ri] - reservePressureWeightPerTask[ix];
                                    if (reservePressureExcludingTask < 0.0)
                                        reservePressureExcludingTask = 0.0;
                                }

                                double criticalReserveExcludingTask = 0.0;
                                if (needResBottleneckPreservation || needResSpecialistMisuse) {
                                    criticalReserveExcludingTask =
                                        criticalReserveByRes[ri] - criticalReserveWeightPerTask[ix];
                                    if (criticalReserveExcludingTask < 0.0)
                                        criticalReserveExcludingTask = 0.0;
                                }

                                double familyMismatchExcludingTask = 0.0;
                                if (needResFamilyMismatch && familyCount > 0) {
                                    const int famId = familyIdByTask[ix];
                                    const size_t base = (size_t)ri * (size_t)familyCount;

                                    double currentFamilyPressure =
                                        familyPressureByResFamily[base + (size_t)famId]
                                        - reservePressureWeightPerTask[ix];
                                    if (currentFamilyPressure < 0.0)
                                        currentFamilyPressure = 0.0;

                                    const double bestOtherFamilyPressure =
                                        (familyBestFamByRes[ri] == famId)
                                        ? familySecondBestPressureByRes[ri]
                                        : familyBestPressureByRes[ri];

                                        familyMismatchExcludingTask =
                                            bestOtherFamilyPressure / (1.0 + currentFamilyPressure);
                                }

                                    Features f = computeResourceFeaturesFast(
                                        I,
                                        ix,
                                        t,
                                        r,
                                        now,
                                        cheapestNow,
                                        cheapestCapableOverall,
                                        waitOfCheapestCapableOverall,
                                        reservePressureExcludingTask,
                                        criticalReserveExcludingTask,
                                        familyMismatchExcludingTask
                                    );

                                double s = ruleR->tree ? ruleR->tree->eval(f) : 0.0;

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

                                double reservePressureExcludingTask = 0.0;
                                if (needResReservePressure) {
                                    reservePressureExcludingTask =
                                        reservePressureByRes[ri] - reservePressureWeightPerTask[ix];
                                    if (reservePressureExcludingTask < 0.0)
                                        reservePressureExcludingTask = 0.0;
                                }

                                double criticalReserveExcludingTask = 0.0;
                                if (needResBottleneckPreservation || needResSpecialistMisuse) {
                                    criticalReserveExcludingTask =
                                        criticalReserveByRes[ri] - criticalReserveWeightPerTask[ix];
                                    if (criticalReserveExcludingTask < 0.0)
                                        criticalReserveExcludingTask = 0.0;
                                }

                                double familyMismatchExcludingTask = 0.0;
                                if (needResFamilyMismatch && familyCount > 0) {
                                    const int famId = familyIdByTask[ix];
                                    const size_t base = (size_t)ri * (size_t)familyCount;

                                    double currentFamilyPressure =
                                        familyPressureByResFamily[base + (size_t)famId]
                                        - reservePressureWeightPerTask[ix];
                                    if (currentFamilyPressure < 0.0)
                                        currentFamilyPressure = 0.0;

                                    const double bestOtherFamilyPressure =
                                        (familyBestFamByRes[ri] == famId)
                                        ? familySecondBestPressureByRes[ri]
                                        : familyBestPressureByRes[ri];

                                        familyMismatchExcludingTask =
                                            bestOtherFamilyPressure / (1.0 + currentFamilyPressure);
                                }

                                Features f = computeResourceFeaturesFast(
                                    I,
                                    ix,
                                    t,
                                    r,
                                    now,
                                    cheapestNow,
                                    cheapestCapableOverall,
                                    waitOfCheapestCapableOverall,
                                    reservePressureExcludingTask,
                                    criticalReserveExcludingTask,
                                    familyMismatchExcludingTask
                                );

                                double s = ruleR->tree ? ruleR->tree->eval(f) : 0.0;

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
                    continue;
                }

                TaskCandidate cand;
                cand.ix = ix;
                cand.allocResId = allocResId;

                if (usePairTree) {
                    cand.taskScore = localBestPairScore;
                    cand.resScore = 0.0;
                    cand.pairScore = localBestPairScore;

                    const bool take = (cand.pairScore < bestScore);

                    if (take) {
                        best = cand.ix;
                        bestResId = cand.allocResId;
                        bestScore = cand.pairScore;
                    }
                }
                else {
                    double sc;
                    if (gpRule) {
                        sc = gpRule->scoreFast(ix, t);
                    }
                    else {
                        sc = ruleT.score(t);
                    }

                    cand.taskScore = sc;
                    cand.resScore = std::isfinite(localBestResScore) ? localBestResScore : 0.0;

                    pairCandidates.push_back(std::move(cand));
                }
            }

            if (!pairCandidates.empty()) {
                if (!usePairTree) {
                    double minTask = std::numeric_limits<double>::infinity();
                    double maxTask = -std::numeric_limits<double>::infinity();
                    double minRes = std::numeric_limits<double>::infinity();
                    double maxRes = -std::numeric_limits<double>::infinity();

                    for (const auto& cand : pairCandidates) {
                        if (std::isfinite(cand.taskScore)) {
                            minTask = std::min(minTask, cand.taskScore);
                            maxTask = std::max(maxTask, cand.taskScore);
                        }
                        if (std::isfinite(cand.resScore)) {
                            minRes = std::min(minRes, cand.resScore);
                            maxRes = std::max(maxRes, cand.resScore);
                        }
                    }

                    for (const auto& cand : pairCandidates) {
                        const double nt = norm01(cand.taskScore, minTask, maxTask);
                        const double nr = norm01(cand.resScore, minRes, maxRes);
                        const double combined = (1.0 - kPairResourceWeight) * nt + kPairResourceWeight * nr;
                        const bool take = (combined < bestScore);

                        if (take) {
                            best = cand.ix;
                            bestResId = cand.allocResId;
                            bestScore = combined;
                        }
                    }
                }
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

                const double scheduledReserveWeight = reservePressureWeightPerTask[best];
                const double scheduledCriticalReserveWeight = criticalReserveWeightPerTask[best];
                const int scheduledFamId = familyIdByTask[best];

                for (int ri : demandCapableResIdxPerTask[best]) {

                    if (scheduledReserveWeight > 0.0) {
                        reservePressureByRes[ri] -= scheduledReserveWeight;
                        if (reservePressureByRes[ri] < 0.0) reservePressureByRes[ri] = 0.0;

                        if (familyCount > 0) {
                            const size_t idx =
                                (size_t)ri * (size_t)familyCount + (size_t)scheduledFamId;
                            familyPressureByResFamily[idx] -= scheduledReserveWeight;
                            if (familyPressureByResFamily[idx] < 0.0) {
                                familyPressureByResFamily[idx] = 0.0;
                            }
                        }
                    }

                    if (scheduledCriticalReserveWeight > 0.0) {
                        criticalReserveByRes[ri] -= scheduledCriticalReserveWeight;
                        if (criticalReserveByRes[ri] < 0.0) criticalReserveByRes[ri] = 0.0;
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
                break;
            }
            int nextT = std::numeric_limits<int>::max();
            for (auto& rt : running) nextT = std::min(nextT, rt.finish);
            advanceTo(nextT);
        }
    }
    clearFeaturePrecomputed();
    clearTaskEvalStepPrecomputed();
    clearPairEvalStepPrecomputed();
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