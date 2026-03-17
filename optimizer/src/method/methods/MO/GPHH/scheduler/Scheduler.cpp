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
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>
#include <iomanip>

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

    struct SpreadStat {
        int count = 0;
        int zeros = 0;
        int ones = 0;
        double minV = std::numeric_limits<double>::infinity();
        double maxV = -std::numeric_limits<double>::infinity();
        std::unordered_set<std::int64_t> uniqRounded;

        void add(double v) {
            if (!std::isfinite(v)) return;
            ++count;
            if (v < minV) minV = v;
            if (v > maxV) maxV = v;
            if (std::abs(v) < 1e-12) ++zeros;
            if (std::abs(v - 1.0) < 1e-12) ++ones;
            uniqRounded.insert((std::int64_t)std::llround(v * 1000.0));
        }

        bool any() const { return count > 0; }
    };

    static void printSpread(const char* name, const SpreadStat& s) {
        if (!s.any()) return;

        std::cout
            << "  " << name
            << "  min=" << s.minV
            << "  max=" << s.maxV
            << "  range=" << (s.maxV - s.minV)
            << "  uniq~=" << s.uniqRounded.size()
            << "  zeros=" << s.zeros;

        if (s.ones > 0) {
            std::cout << "  ones=" << s.ones;
        }
        std::cout << "\n";
    }

    struct TaskSpreadTrace {
        SpreadStat score;
        SpreadStat avail;
        SpreadStat gap;
        SpreadStat wait;
        SpreadStat crit;
        SpreadStat slack;
        SpreadStat succ;
        SpreadStat desc;
        SpreadStat tpred;
        SpreadStat critPress;
        SpreadStat minCostNow;
        SpreadStat regretNow;

        void add(double sc, const Features& f) {
            score.add(sc);
            avail.add(f.availSkill);
            gap.add(f.availGap);
            wait.add(f.waitRes);
            crit.add(f.critLen);
            slack.add(f.slack);
            succ.add(f.succCount);
            desc.add(f.descCount);
            tpred.add(f.totPred);
            critPress.add(f.criticalPressure);
            minCostNow.add(f.minFeasibleCostNow);
            regretNow.add(f.costRegretNow);
        }

        void print() const {
            std::cout << "READY_TASK_SPREAD\n";
            printSpread("TASK_SCORE", score);
            printSpread("AVAIL", avail);
            printSpread("GAP", gap);
            printSpread("WAIT", wait);
            printSpread("CRITLEN", crit);
            printSpread("SLACK", slack);
            printSpread("SUCC", succ);
            printSpread("DESC", desc);
            printSpread("TPRED", tpred);
            printSpread("CRIT_PRESS", critPress);
            printSpread("MIN_COST_NOW", minCostNow);
            printSpread("REGRET_NOW", regretNow);
        }
    };

    struct ResourceSpreadTrace {
        int candidates = 0;

        SpreadStat score;
        SpreadStat wait;
        SpreadStat canNow;
        SpreadStat assignCost;
        SpreadStat assignPremiumAll;
        SpreadStat haste;
        SpreadStat reserve;
        SpreadStat critReserve;
        SpreadStat stratMismatch;
        SpreadStat familyMismatch;
        SpreadStat relWage;
        SpreadStat surplus;
        SpreadStat util;

        void add(double sc, const Features& f) {
            ++candidates;
            score.add(sc);
            wait.add(f.resWaitTime);
            canNow.add(f.resCanStartNow);
            assignCost.add(f.resAssignCost);
            assignPremiumAll.add(f.resAssignPremiumAll);
            haste.add(f.resHasteValue);
            reserve.add(f.resReservePressure);
            critReserve.add(f.resCriticalReserve);
            stratMismatch.add(f.resStrategicMismatch);
            familyMismatch.add(f.resFamilyMismatch);
            relWage.add(f.resRelativeWage);
            surplus.add(f.resSurplusLevel);
            util.add(f.resUtilization);
        }

        void print(int taskId) const {
            std::cout << "RESOURCE_SPREAD for T" << taskId
                << "  candidates=" << candidates << "\n";
            printSpread("RES_SCORE", score);
            printSpread("RES_WAIT", wait);
            printSpread("RES_CAN_NOW", canNow);
            printSpread("RES_COST", assignCost);
            printSpread("RES_PREMIUM_ALL", assignPremiumAll);
            printSpread("RES_HASTE", haste);
            printSpread("RES_RESERVE", reserve);
            printSpread("RES_CRIT_RES", critReserve);
            printSpread("RES_STR_MIS", stratMismatch);
            printSpread("RES_FAM_MIS", familyMismatch);
            printSpread("RES_REL_WAGE", relWage);
            printSpread("RES_SURPLUS", surplus);
            printSpread("RES_UTIL", util);
        }
    };

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

        {
            std::unordered_map<std::string, int> familyIndex;
            familyIndex.reserve((size_t)n * 2);

            for (int ui = 0; ui < n; ++ui) {
                const Task& u = I.tasks[ui];
                const std::string key = u.reqSkill + "#" + std::to_string(std::max(0, u.reqLevel));

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
            const int reqU = std::max(0, u.reqLevel);

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
                    auto jt = rr.skills.find(u.reqSkill);
                    const int ll = (jt != rr.skills.end()) ? jt->second : 0;
                    if (reqU > 0 && ll < reqU) continue;

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
            for (int ri = 0; ri < (int)I.resources.size(); ++ri) {
                const auto& rr = I.resources[ri];
                auto jt = rr.skills.find(u.reqSkill);
                const int ll = (jt != rr.skills.end()) ? jt->second : 0;
                if (ll >= reqU) {
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
        g_staticCache.reservePressureByResInit.size() != I.resources.size() ||
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
    const GPTreeRule* gpRule = dynamic_cast<const GPTreeRule*>(&ruleT);

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
            TaskSpreadTrace readySpread;
            const_cast<IDispatchingRule&>(ruleT).setContext(&I, now);

            const GPTreeRule* gp = nullptr;
            if (g_trace) {
                gp = gpRule;
                if (gp) {
                    std::cout << "\n[time now=" << now << "]\n";
                    std::cout << "TASK_RULE = " << gp->exprString() << "\n";
                    std::cout << "RES_RULE  = " << ((ruleR && ruleR->tree) ? ruleR->tree->toString() : std::string("NONE")) << "\n";
                    std::cout << "ID  DUR  REQ  AVAIL  GAP  WAIT  EST  CRITLEN  SLACK  SUCC  TPRED   SCORE   BEST_RES  RES_SCORE\n";
                }
                else {
                    std::cout << "\n[time now=" << now << "]  (trace dostępny tylko dla GPTreeRule)\n";
                }
            }

            constexpr double kPairResourceWeight = 0.1;

            int best = -1;
            int bestResId = -1;
            double bestScore = std::numeric_limits<double>::infinity();
            double bestTaskScore = std::numeric_limits<double>::infinity();
            double bestResScore = std::numeric_limits<double>::infinity();
            int minWaitFeasible = std::numeric_limits<int>::max() / 4;

            struct ResTraceRow {
                int resId = -1;
                double score = std::numeric_limits<double>::infinity();
                Features feat;
            };

            struct TaskCandidate {
                int ix = -1;
                int allocResId = -1;
                double taskScore = std::numeric_limits<double>::infinity();
                double resScore = std::numeric_limits<double>::infinity();
                double pairScore = std::numeric_limits<double>::infinity();
                std::vector<ResTraceRow> topRes;
                ResourceSpreadTrace resSpread;
            };

            auto pushTop3Res = [](std::vector<ResTraceRow>& rows, int resId, double score, const Features& feat) {
                rows.push_back(ResTraceRow{ resId, score, feat });
                std::sort(rows.begin(), rows.end(),
                    [](const ResTraceRow& a, const ResTraceRow& b) {
                        return a.score < b.score;
                    });
                if (rows.size() > 3) rows.resize(3);
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

            std::vector<ResTraceRow> bestTaskTopRes;
            ResourceSpreadTrace bestTaskResSpread;
            std::vector<TaskCandidate> pairCandidates;
            pairCandidates.reserve(ready.size());

            for (int ix : ready) {
                Task& t = I.tasks[ix];
                const int req = t.reqLevel;

                std::vector<ResTraceRow> localTopRes;
                ResourceSpreadTrace localResSpread;
                int localBestResId = -1;
                double localBestResScore = 0.0;

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
                        else {
                            localBestResId = allocResId;
                            localBestResScore = 0.0;
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

                                double reservePressureExcludingTask =
                                    reservePressureByRes[ri] - reservePressureWeightPerTask[ix];
                                if (reservePressureExcludingTask < 0.0)
                                    reservePressureExcludingTask = 0.0;

                                double criticalReserveExcludingTask =
                                    criticalReserveByRes[ri] - criticalReserveWeightPerTask[ix];
                                if (criticalReserveExcludingTask < 0.0)
                                    criticalReserveExcludingTask = 0.0;

                                double familyMismatchExcludingTask = 0.0;
                                if (familyCount > 0) {
                                    const int famId = familyIdByTask[ix];
                                    const size_t base = (size_t)ri * (size_t)familyCount;

                                    double currentFamilyPressure =
                                        familyPressureByResFamily[base + (size_t)famId]
                                        - reservePressureWeightPerTask[ix];
                                    if (currentFamilyPressure < 0.0)
                                        currentFamilyPressure = 0.0;

                                    double bestOtherFamilyPressure = 0.0;
                                    for (int f = 0; f < familyCount; ++f) {
                                        if (f == famId) continue;
                                        bestOtherFamilyPressure = std::max(
                                            bestOtherFamilyPressure,
                                            familyPressureByResFamily[base + (size_t)f]
                                        );
                                    }

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

                                if (g_trace) {
                                    pushTop3Res(localTopRes, r.id, s, f);
                                    localResSpread.add(s, f);
                                }

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

                                double reservePressureExcludingTask =
                                    reservePressureByRes[ri] - reservePressureWeightPerTask[ix];
                                if (reservePressureExcludingTask < 0.0)
                                    reservePressureExcludingTask = 0.0;

                                double criticalReserveExcludingTask =
                                    criticalReserveByRes[ri] - criticalReserveWeightPerTask[ix];
                                if (criticalReserveExcludingTask < 0.0)
                                    criticalReserveExcludingTask = 0.0;

                                double familyMismatchExcludingTask = 0.0;
                                if (familyCount > 0) {
                                    const int famId = familyIdByTask[ix];
                                    const size_t base = (size_t)ri * (size_t)familyCount;

                                    double currentFamilyPressure =
                                        familyPressureByResFamily[base + (size_t)famId]
                                        - reservePressureWeightPerTask[ix];
                                    if (currentFamilyPressure < 0.0)
                                        currentFamilyPressure = 0.0;

                                    double bestOtherFamilyPressure = 0.0;
                                    for (int f = 0; f < familyCount; ++f) {
                                        if (f == famId) continue;
                                        bestOtherFamilyPressure = std::max(
                                            bestOtherFamilyPressure,
                                            familyPressureByResFamily[base + (size_t)f]
                                        );
                                    }

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

                                if (g_trace) {
                                    pushTop3Res(localTopRes, r.id, s, f);
                                    localResSpread.add(s, f);
                                }

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
                            << "    " << 1e12
                            << "    " << -1
                            << "    " << 1e12
                            << "  (X)\n";
                    }
                    continue;
                }

                double sc;
                if (gp) {
                    ScoreTrace tr = gp->scoreWithTraceFast(ix, t);
                    sc = tr.score;

                    if (g_trace) {
                        readySpread.add(sc, tr.feat);

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
                            << "    " << localBestResId
                            << "    " << (std::isfinite(localBestResScore) ? localBestResScore : 1e12)
                            << "\n";
                    }
                }
                else {
                    if (gpRule) {
                        sc = gpRule->scoreFast(ix, t);
                    }
                    else {
                        sc = ruleT.score(t);
                    }
                }

                TaskCandidate cand;
                cand.ix = ix;
                cand.allocResId = allocResId;
                cand.taskScore = sc;
                cand.resScore = std::isfinite(localBestResScore) ? localBestResScore : 0.0;
                cand.topRes = std::move(localTopRes);
                cand.resSpread = std::move(localResSpread);

                pairCandidates.push_back(std::move(cand));
            }

            if (!pairCandidates.empty()) {
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

                for (auto& cand : pairCandidates) {
                    const double taskNorm = norm01(cand.taskScore, minTask, maxTask);
                    const double resNorm = (ruleR != nullptr)
                        ? norm01(cand.resScore, minRes, maxRes)
                        : 0.0;

                    cand.pairScore =
                        (1.0 - kPairResourceWeight) * taskNorm +
                        kPairResourceWeight * resNorm;

                    bool take = false;

                    if (cand.pairScore < bestScore) {
                        take = true;
                    }
                    else if (best != -1 && std::abs(cand.pairScore - bestScore) < 1e-12) {
                        if (cand.taskScore < bestTaskScore - 1e-12) {
                            take = true;
                        }
                        else if (std::abs(cand.taskScore - bestTaskScore) < 1e-12 &&
                            cand.resScore < bestResScore - 1e-12) {
                            take = true;
                        }
                        else if (g_priorityKeys) {
                            const auto& K = *g_priorityKeys;
                            if ((size_t)best < K.size() &&
                                (size_t)cand.ix < K.size() &&
                                K[cand.ix] < K[best]) {
                                take = true;
                            }
                        }
                    }

                    if (take) {
                        best = cand.ix;
                        bestResId = cand.allocResId;
                        bestScore = cand.pairScore;
                        bestTaskScore = cand.taskScore;
                        bestResScore = cand.resScore;
                        bestTaskTopRes = cand.topRes;
                        bestTaskResSpread = cand.resSpread;
                    }
                }
            }

            if (g_trace && best != -1) {
                std::cout << "=> wybieram T" << I.tasks[best].id
                    << "  (pairScore=" << bestScore
                    << ", taskScore=" << bestTaskScore
                    << ", resScore=" << bestResScore
                    << ", bestRes=" << bestResId << ")\n";

                if (!bestTaskTopRes.empty()) {
                    std::cout << "TOP_RESOURCES for T" << I.tasks[best].id
                        << " : ID  SCORE  WAIT  ASSIGN_COST  ASSIGN_PREM_ALL  HASTE  RESERVE  CRIT_RES  STR_MIS  FAM_MIS  CAN_NOW\n";

                    for (const auto& rr : bestTaskTopRes) {
                        std::cout << "R" << rr.resId
                            << "  " << rr.score
                            << "  " << rr.feat.resWaitTime
                            << "  " << rr.feat.resAssignCost
                            << "  " << rr.feat.resAssignPremiumAll
                            << "  " << rr.feat.resHasteValue
                            << "  " << rr.feat.resReservePressure
                            << "  " << rr.feat.resCriticalReserve
                            << "  " << rr.feat.resStrategicMismatch
                            << "  " << rr.feat.resFamilyMismatch
                            << "  " << rr.feat.resCanStartNow
                            << "\n";
                    }
                }
                readySpread.print();
                bestTaskResSpread.print(I.tasks[best].id);
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