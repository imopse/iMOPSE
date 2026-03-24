#include "Features.hpp"
#include "FeatureScaling.hpp"
#include <algorithm>
#include <limits>
#include <cmath>
#include <cstdint>
#include "Precompute.hpp"
#include "../alloc/ResourceAllocator.hpp"

namespace {

    const std::vector<double>* g_taskResCountByTask = nullptr;
    const std::vector<double>* g_avgResCostByTask = nullptr;
    const std::unordered_map<int, int>* g_resIndexById = nullptr;
    const int* g_unschedCountPtr = nullptr;
    const std::vector<int>* g_remainingPredCountByTask = nullptr;
    const std::vector<int>* g_latestPredFinishByTask = nullptr;
    const std::unordered_map<std::string, SkillStepInfo>* g_skillStepCache = nullptr;
    const std::vector<int>* g_matchedLevelByTaskRes = nullptr;
    int g_matchedLevelResCount = 0;

    struct ResourceStepBase {
        double resWage = 0.0;
        double resIdleTime = 0.0;
        double resCanStartNow = 0.0;
        double resUtilization = 0.0;
    };

    std::vector<Features> g_taskStepFeatures;
    std::vector<std::uint64_t> g_taskStepStamp;
    std::uint64_t g_taskStepCurrentStamp = 1;
    bool g_taskEvalStepReady = false;

    std::vector<Features> g_pairBaseTaskFeatures;
    std::vector<std::uint64_t> g_pairBaseTaskStamp;
    std::vector<ResourceStepBase> g_pairBaseResourceFeatures;

    std::vector<double> g_pairFutureBranchFitCache;
    std::vector<std::uint64_t> g_pairFutureBranchFitStamp;
    std::uint64_t g_pairCurrentStamp = 1;
    int g_pairFutureBranchFitResCount = 0;

    bool g_pairEvalStepReady = false;
    bool g_needResFutureBranchFit = true;
    bool g_needResBottleneckPreservation = true;
    bool g_needResSpecialistMisuse = true;
    bool g_needTaskStructureFeatures = true;
    bool g_needTaskResCount = true;
    bool g_needTaskAvgResCost = true;
    bool g_needTaskUnschedTasks = true;
    bool g_needTaskAvailabilityFeatures = true;
    bool g_needTaskCostNowFeatures = true;
    bool g_needTaskReleasePressure = true;
    bool g_needTaskCriticalPressure = true;

    std::size_t g_descCacheTaskSig = 0;
    int g_descCacheTaskCount = -1;
    std::vector<std::vector<int>> g_descendantsByTask;

    std::size_t g_nearDescCacheTaskSig = 0;
    int g_nearDescCacheTaskCount = -1;
    std::vector<std::vector<int>> g_nearDescendantsByTask;

    inline double inf() { return std::numeric_limits<double>::infinity(); }

    inline double clamp01(double x) {
        if (!std::isfinite(x)) return 0.0;
        if (x < 0.0) return 0.0;
        if (x > 1.0) return 1.0;
        return x;
    }

    inline double normalize(double val, double maxVal) {
        if (!std::isfinite(val)) return 1.0;
        if (maxVal <= 0.0)       return 0.0;
        double x = val / maxVal;
        if (x < 0.0) x = 0.0;
        if (x > 1.0) x = 1.0;
        return x;
    }

    inline std::uint64_t nextStamp(std::uint64_t& current) {
        ++current;

        if (current == 0) {
            current = 1;
        }

        return current;
    }

    inline bool predecessorsDoneNow(const Instance& I, const Task& t, int now) {
        for (int pid : t.predecessors) {
            auto it = I.idToIndex.find(pid);
            if (it != I.idToIndex.end()) {
                const Task& p = I.tasks[it->second];
                if (p.finish < 0 || p.finish > now) return false;
            }
        }
        return true;
    }

    inline double latestPredFinish(const Instance& I, const Task& t) {
        int mx = 0;
        for (int pid : t.predecessors) {
            auto it = I.idToIndex.find(pid);
            if (it != I.idToIndex.end()) {
                const Task& p = I.tasks[it->second];
                if (p.finish > mx) mx = p.finish;
            }
        }
        return (double)mx;
    }

    inline int countUnschedTasks(const Instance& I) {
        if (g_unschedCountPtr) return *g_unschedCountPtr;

        int unsched = 0;
        for (const auto& tt : I.tasks) if (tt.start < 0) ++unsched;
        return unsched;
    }

    inline bool canUseSingleSkillCache(const Task& t) {
        return t.requiredSkills.empty() || t.requiredSkills.size() == 1;
    }

    inline int totalReq(const Task& t) {
        return std::max(0, t.totalRequiredLevel());
    }
    inline int matchedLevelCached(
        const Instance& I,
        int taskIx,
        const Task& t,
        const Resource& r)
    {
        if (g_matchedLevelByTaskRes &&
            g_matchedLevelResCount > 0 &&
            g_resIndexById &&
            taskIx >= 0) {
            auto it = g_resIndexById->find(r.id);
            if (it != g_resIndexById->end()) {
                const int ri = it->second;
                const size_t idx =
                    (size_t)taskIx * (size_t)g_matchedLevelResCount + (size_t)ri;

                if (idx < g_matchedLevelByTaskRes->size()) {
                    return (*g_matchedLevelByTaskRes)[idx];
                }
            }
        }

        return t.matchedLevelOn(r);
    }

    inline int resourceIndexCached(const Resource& r) {
        if (!g_resIndexById) return -1;

        auto it = g_resIndexById->find(r.id);
        if (it == g_resIndexById->end()) return -1;

        return it->second;
    }

    inline ResourceStepBase computeResourceStepBaseRaw(
        const Resource& r,
        int now)
    {
        ResourceStepBase base{};

        base.resWage = r.salary;
        base.resIdleTime = (r.busyUntil < now) ? (double)(now - r.busyUntil) : 0.0;
        base.resCanStartNow = (r.busyUntil <= now) ? 1.0 : 0.0;

        double busySoFar = (double)r.totalBusy;
        if (r.busy && now > r.busyStart) {
            busySoFar += (double)(now - r.busyStart);
        }

        base.resUtilization = (now > 0) ? (busySoFar / (double)now) : 0.0;
        return base;
    }

    inline bool tryGetResourceStepBaseCached(
        const Resource& r,
        ResourceStepBase& outBase)
    {
        if (!g_pairEvalStepReady) return false;

        const int resIx = resourceIndexCached(r);
        if (resIx < 0) return false;
        if (resIx >= (int)g_pairBaseResourceFeatures.size()) return false;

        outBase = g_pairBaseResourceFeatures[resIx];
        return true;
    }

    inline void assignResourceStepBaseToFeatures(
        const ResourceStepBase& base,
        Features& f)
    {
        f.resWage = base.resWage;
        f.resIdleTime = base.resIdleTime;
        f.resCanStartNow = base.resCanStartNow;
        f.resUtilization = base.resUtilization;
    }

    inline bool tryGetFutureBranchFitStepCached(
        int taskIx,
        const Resource& r,
        double& outValue
    ) {
        if (!g_pairEvalStepReady) return false;
        if (taskIx < 0) return false;
        if (g_pairFutureBranchFitResCount <= 0) return false;

        const int resIx = resourceIndexCached(r);
        if (resIx < 0 || resIx >= g_pairFutureBranchFitResCount) return false;

        const size_t flatIx =
            (size_t)taskIx * (size_t)g_pairFutureBranchFitResCount + (size_t)resIx;

        if (flatIx >= g_pairFutureBranchFitStamp.size()) return false;
        if (g_pairFutureBranchFitStamp[flatIx] != g_pairCurrentStamp) return false;

        outValue = g_pairFutureBranchFitCache[flatIx];
        return true;
    }


    inline double bestFreeMatchedLevel(const Instance& I, int taskIx, const Task& t, int now) {
        double best = 0.0;

        if (!t.capableResourceIndices.empty()) {
            for (int ri : t.capableResourceIndices) {
                if (ri < 0 || ri >= (int)I.resources.size()) continue;
                const auto& r = I.resources[ri];
                if (r.busyUntil > now) continue;
                best = std::max(best, (double)matchedLevelCached(I, taskIx, t, r));
            }
            return best;
        }

        for (const auto& r : I.resources) {
            if (r.busyUntil > now) continue;
            if (!t.canBeDoneBy(r)) continue;
            best = std::max(best, (double)matchedLevelCached(I, taskIx, t, r));
        }

        return best;
    }

    inline double bestWaitForTask(const Instance& I, const Task& t, int now) {
        const int req = totalReq(t);
        if (req <= 0) return 0.0;

        int bestWait = std::numeric_limits<int>::max();

        if (!t.capableResourceIndices.empty()) {
            for (int ri : t.capableResourceIndices) {
                if (ri < 0 || ri >= (int)I.resources.size()) continue;
                const auto& r = I.resources[ri];
                if (r.busyUntil <= now) return 0.0;
                bestWait = std::min(bestWait, r.busyUntil - now);
            }
            return (double)bestWait;
        }

        for (const auto& r : I.resources) {
            if (!t.canBeDoneBy(r)) continue;
            if (r.busyUntil <= now) return 0.0;
            bestWait = std::min(bestWait, r.busyUntil - now);
        }

        return (double)bestWait;
    }

    inline double taskResCountFeature(const Instance& I, int taskIx, const Task& t, int req) {
        if (g_taskResCountByTask && taskIx >= 0 && taskIx < (int)g_taskResCountByTask->size()) {
            return (*g_taskResCountByTask)[taskIx];
        }

        if (req <= 0) return (double)I.resources.size();

        if (!t.capableResources.empty()) {
            return (double)t.capableResources.size();
        }

        int cnt = 0;
        for (const auto& r : I.resources) {
            if (t.canBeDoneBy(r)) ++cnt;
        }
        return (double)cnt;
    }

    inline double avgSalaryForSkill(const Instance& I, int taskIx, const Task& t, int req) {
        if (g_avgResCostByTask && taskIx >= 0 && taskIx < (int)g_avgResCostByTask->size()) {
            return (*g_avgResCostByTask)[taskIx];
        }

        if (req <= 0) return inf();

        if (!t.capableResourceIndices.empty()) {
            double sumSal = 0.0;
            int cnt = 0;
            for (int ri : t.capableResourceIndices) {
                if (ri < 0 || ri >= (int)I.resources.size()) continue;
                sumSal += I.resources[ri].salary;
                ++cnt;
            }
            return (cnt > 0) ? (sumSal / (double)cnt) : inf();
        }

        double sumSal = 0.0;
        int cnt = 0;
        for (const auto& r : I.resources) {
            if (!t.canBeDoneBy(r)) continue;
            sumSal += r.salary;
            ++cnt;
        }
        return (cnt > 0) ? (sumSal / (double)cnt) : inf();
    }

    inline const SkillStepInfo* skillStepInfoFor(const std::string& skill) {
        if (!g_skillStepCache) return nullptr;
        auto it = g_skillStepCache->find(skill);
        if (it == g_skillStepCache->end()) return nullptr;
        return &it->second;
    }

    inline double cachedAvailSkill(const std::string& skill) {
        const SkillStepInfo* info = skillStepInfoFor(skill);
        if (!info) return -1.0;
        return (double)info->maxFreeLevel;
    }

    inline double cachedWaitRes(const std::string& skill, int req) {
        const SkillStepInfo* info = skillStepInfoFor(skill);
        if (!info || req <= 0) return -1.0;
        if (req >= (int)info->minWaitAtLeast.size()) return (double)std::numeric_limits<int>::max();
        return (double)info->minWaitAtLeast[req];
    }

    inline bool cachedCheapestPair(const std::string& skill, int req, double& first, double& second) {
        const SkillStepInfo* info = skillStepInfoFor(skill);
        if (!info || req <= 0) return false;
        if (req >= (int)info->cheapestAtLeast.size()) return false;

        first = info->cheapestAtLeast[req];
        second = info->secondCheapestAtLeast[req];

        return std::isfinite(first);
    }


    inline void fillCostNowFeatures(Features& f, const Instance& I, const Task& t, int req, int now) {
        if (!f.feasibleNow) {
            f.cheapestCostNow = inf();
            f.costPerSkillNow = inf();
            f.minFeasibleCostNow = inf();
            f.costRegretNow = 0.0;
            return;
        }

        if (canUseSingleSkillCache(t) && t.capableResources.empty() && req > 0 && !t.reqSkill.empty()) {
            double first = std::numeric_limits<double>::infinity();
            double second = std::numeric_limits<double>::infinity();

            if (cachedCheapestPair(t.reqSkill, req, first, second)) {
                if (!std::isfinite(second)) second = first;

                f.cheapestCostNow = first;
                f.costPerSkillNow = first / (double)std::max(1, req);
                f.minFeasibleCostNow = first * (double)t.duration;
                f.costRegretNow = (second - first) * (double)t.duration;
                return;
            }
        }

        double first = std::numeric_limits<double>::infinity();
        double second = std::numeric_limits<double>::infinity();
        bool found = false;

        auto considerSalary = [&](double sal) {
            found = true;
            if (sal < first) {
                second = first;
                first = sal;
            }
            else if (sal < second) {
                second = sal;
            }
            };

        if (!t.capableResourceIndices.empty()) {
            for (int ri : t.capableResourceIndices) {
                if (ri < 0 || ri >= (int)I.resources.size()) continue;

                const auto& r = I.resources[ri];
                if (r.busyUntil > now) continue;

                considerSalary(r.salary);
            }
        }
        else {
            for (const auto& r : I.resources) {
                if (r.busyUntil > now) continue;
                if (!t.canBeDoneBy(r)) continue;
                considerSalary(r.salary);
            }
        }

        if (!found || !std::isfinite(first)) {
            f.cheapestCostNow = inf();
            f.costPerSkillNow = inf();
            f.minFeasibleCostNow = inf();
            f.costRegretNow = 0.0;
            return;
        }

        if (!std::isfinite(second)) second = first;

        f.cheapestCostNow = first;
        f.costPerSkillNow = first / (double)std::max(1, req);
        f.minFeasibleCostNow = first * (double)t.duration;
        f.costRegretNow = (second - first) * (double)t.duration;
    }

    inline double bottleneckPreservationRaw(
        const Instance& I,
        int taskIx,
        const Task& t,
        double criticalReserveExcludingTask)
    {
        if (criticalReserveExcludingTask <= 0.0) return 0.0;

        const auto& S = gp::getFeatureScaling();
        const int req = std::max(0, totalReq(t));
        const double feasibleCount = taskResCountFeature(I, taskIx, t, req);

        double critNorm = 0.0;
        double slackNorm = 0.0;

        if (auto cpm = gp::getCPMPrecalc()) {
            if (taskIx >= 0 && taskIx < (int)cpm->critLen.size() && S.maxCritLen > 0.0) {
                critNorm = std::min(1.0, (double)cpm->critLen[taskIx] / S.maxCritLen);
            }
            if (taskIx >= 0 && taskIx < (int)cpm->slack.size() && S.maxSlackPos > 0.0) {
                slackNorm = std::min(
                    1.0,
                    std::max(0.0, (double)cpm->slack[taskIx]) / S.maxSlackPos
                );
            }
        }

        const double reqSafe = (double)std::max(1, req);
        const double replaceability = feasibleCount / reqSafe;

        const double easyFactor =
            replaceability * (1.0 + slackNorm) / (1.0 + critNorm);

        return criticalReserveExcludingTask * easyFactor;
    }

    inline double specialistMisuseRaw(
        const Instance& I,
        int taskIx,
        const Task& t,
        const Resource& r,
        double criticalReserveExcludingTask)
    {
        if (criticalReserveExcludingTask <= 0.0) return 0.0;

        const auto& S = gp::getFeatureScaling();
        const int req = std::max(0, totalReq(t));
        const double reqSafe = (double)std::max(1, req);
        const double feasibleCount = taskResCountFeature(I, taskIx, t, req);

        const double matched = (double)std::max(0, matchedLevelCached(I, taskIx, t, r));
        const double overkill =
            std::max(0.0, matched - reqSafe) / reqSafe;

        double critNorm = 0.0;
        double slackNorm = 0.0;

        if (auto cpm = gp::getCPMPrecalc()) {
            if (taskIx >= 0 && taskIx < (int)cpm->critLen.size() && S.maxCritLen > 0.0) {
                critNorm = std::min(1.0, (double)cpm->critLen[taskIx] / S.maxCritLen);
            }
            if (taskIx >= 0 && taskIx < (int)cpm->slack.size() && S.maxSlackPos > 0.0) {
                slackNorm = std::min(
                    1.0,
                    std::max(0.0, (double)cpm->slack[taskIx]) / S.maxSlackPos
                );
            }
        }

        const double replaceability = feasibleCount / reqSafe;
        const double easyFactor = (1.0 + slackNorm) / (1.0 + critNorm);

        return criticalReserveExcludingTask
            * replaceability
            * easyFactor
            * (1.0 + overkill);
    }

    inline double taskReleasePressureRaw(
        const Task& t,
        double taskResCountRaw,
        double critLenRaw,
        double slackRaw,
        double descCountRaw)
    {
        const double reqRaw = (double)std::max(1, totalReq(t));
        const double scarcity = reqRaw / std::max(1.0, taskResCountRaw);
        const double urgency = (1.0 + std::max(0.0, critLenRaw))
            / (1.0 + std::max(0.0, slackRaw));
        const double branching = 1.0 + std::max(0.0, descCountRaw);

        return branching * urgency * scarcity;
    }

    inline double fitGapRatioRaw(
        const Instance& I,
        int taskIx,
        const Task& t,
        const Resource& r)
    {
        const int req = totalReq(t);
        if (req <= 0) return 0.0;

        const int lvl = std::max(0, matchedLevelCached(I, taskIx, t, r));
        if (lvl < req) return inf();

        return (double)std::max(0, lvl - req) / (double)std::max(1, req);
    }

    inline double feasibleCheapnessRankRaw(
        const Instance& I,
        const Task& t,
        const Resource& r)
    {
        if (!t.canBeDoneBy(r)) return 0.0;

        int feasibleCount = 0;
        int cheaperCount = 0;
        int equalCount = 0;

        auto scanRes = [&](const Resource& rr) {
            if (!t.canBeDoneBy(rr)) return;

            ++feasibleCount;

            if (rr.salary < r.salary) {
                ++cheaperCount;
            }
            else if (rr.salary == r.salary) {
                ++equalCount;
            }
            };

        if (!t.capableResourceIndices.empty()) {
            for (int ri : t.capableResourceIndices) {
                if (ri < 0 || ri >= (int)I.resources.size()) continue;
                scanRes(I.resources[ri]);
            }
        }
        else {
            for (const auto& rr : I.resources) {
                scanRes(rr);
            }
        }

        if (feasibleCount <= 1) return 1.0;

        const double avgRankZeroBased =
            (double)cheaperCount + 0.5 * (double)std::max(0, equalCount - 1);

        return clamp01(
            1.0 - avgRankZeroBased / (double)(feasibleCount - 1)
        );
    }


    inline bool usedResourceOnTask(const Task& t, int resId) {
        return std::find(t.assignedResources.begin(), t.assignedResources.end(), resId)
            != t.assignedResources.end();
    }

    inline void normalizeTaskFeatures(Features& f, const gp::FeatureScaling& S) {
        f.duration = normalize(f.duration, S.maxDuration);
        f.reqLevel = normalize(f.reqLevel, S.maxReqLevel);
        f.taskResCount = normalize(f.taskResCount, S.maxTaskResCount);
        f.avgResCostForSkill = normalize(f.avgResCostForSkill, S.maxAvgResCostForSkill);
        f.unschedTasks = normalize(f.unschedTasks, S.maxUnschedTasks);
        f.availSkill = normalize(f.availSkill, S.maxAvailSkill);

        const double minGap = -S.maxReqLevel;
        const double maxGap = S.maxAvailGapPos;
        if (!std::isfinite(f.availGap)) {
            f.availGap = 1.0;
        }
        else {
            double x = (f.availGap - minGap) / (maxGap - minGap);
            if (x < 0.0) x = 0.0;
            if (x > 1.0) x = 1.0;
            f.availGap = x;
        }

        f.critLen = normalize(f.critLen, S.maxCritLen);
        f.slack = normalize(f.slack, S.maxSlackPos);
        f.descCount = normalize(f.descCount, S.maxDescCount);
        f.taskReleasePressure = normalize(f.taskReleasePressure, S.maxTaskReleasePressure);

        f.cheapestCostNow = normalize(f.cheapestCostNow, S.maxCheapestCostNow);
        f.costPerSkillNow = normalize(f.costPerSkillNow, S.maxCostPerSkillNow);

        f.minFeasibleCostNow = normalize(f.minFeasibleCostNow, S.maxMinFeasibleCostNow);
        f.costRegretNow = normalize(f.costRegretNow, S.maxCostRegretNow);
    }

    inline double amplifyBusyWaitPenalty(
        double rawWait,
        bool canStartNow,
        double critNorm,
        double slackNorm,
        const gp::FeatureScaling& S)
    {
        if (canStartNow || rawWait <= 0.0) {
            return 0.0;
        }

        double waitNorm = normalize(rawWait, S.maxWaitRes);
        if (waitNorm < 0.0) waitNorm = 0.0;
        if (waitNorm > 1.0) waitNorm = 1.0;

        double softened = 0.55 * waitNorm + 0.20 * std::sqrt(waitNorm);

        double critSafe = critNorm;
        if (critSafe < 0.0) critSafe = 0.0;
        if (critSafe > 1.0) critSafe = 1.0;

        double slackSafe = slackNorm;
        if (slackSafe < 0.0) slackSafe = 0.0;
        if (slackSafe > 1.0) slackSafe = 1.0;

        double urgency = 0.60 * critSafe + 0.40 * (1.0 - slackSafe);
        if (urgency < 0.0) urgency = 0.0;
        if (urgency > 1.0) urgency = 1.0;

        double floorPenalty = 0.02 + 0.08 * urgency;

        return std::max(softened, floorPenalty);
    }

    inline void ensureDescendantCache(const Instance& I) {
        const int N = (int)I.tasks.size();
        const std::size_t sig = I.taskStructureSignatureReady ? I.taskStructureSignature : 0;

        if ((int)g_descendantsByTask.size() == N) {
            if ((sig != 0 && g_descCacheTaskSig == sig) ||
                (sig == 0 && g_descCacheTaskCount == N)) {
                return;
            }
        }

        std::vector<std::vector<int>> succ(N);
        for (int u = 0; u < N; ++u) {
            for (int pid : I.tasks[u].predecessors) {
                auto it = I.idToIndex.find(pid);
                if (it != I.idToIndex.end()) {
                    succ[it->second].push_back(u);
                }
            }
        }

        g_descendantsByTask.assign(N, {});

        for (int v = 0; v < N; ++v) {
            std::vector<unsigned char> seen(N, 0);
            std::vector<int> stack;

            for (int s : succ[v]) {
                stack.push_back(s);
            }

            while (!stack.empty()) {
                int u = stack.back();
                stack.pop_back();

                if (u < 0 || u >= N) continue;
                if (seen[u]) continue;

                seen[u] = 1;
                g_descendantsByTask[v].push_back(u);

                for (int s : succ[u]) {
                    stack.push_back(s);
                }
            }
        }

        g_descCacheTaskSig = sig;
        g_descCacheTaskCount = N;
    }

    inline void ensureNearDescendantCache(const Instance& I) {
        const int N = (int)I.tasks.size();
        const std::size_t sig = I.taskStructureSignatureReady ? I.taskStructureSignature : 0;

        if ((int)g_nearDescendantsByTask.size() == N) {
            if ((sig != 0 && g_nearDescCacheTaskSig == sig) ||
                (sig == 0 && g_nearDescCacheTaskCount == N)) {
                return;
            }
        }

        std::vector<std::vector<int>> succ(N);
        for (int u = 0; u < N; ++u) {
            for (int pid : I.tasks[u].predecessors) {
                auto it = I.idToIndex.find(pid);
                if (it != I.idToIndex.end()) {
                    succ[it->second].push_back(u);
                }
            }
        }

        g_nearDescendantsByTask.assign(N, {});

        struct NodeDepth {
            int node;
            int depth;
        };

        constexpr int MAX_DEPTH = 3;

        for (int v = 0; v < N; ++v) {
            std::vector<unsigned char> seen(N, 0);
            std::vector<NodeDepth> stack;

            for (int s : succ[v]) {
                stack.push_back({ s, 1 });
            }

            while (!stack.empty()) {
                NodeDepth cur = stack.back();
                stack.pop_back();

                if (cur.node < 0 || cur.node >= N) continue;
                if (cur.depth > MAX_DEPTH) continue;
                if (seen[cur.node]) continue;

                seen[cur.node] = 1;
                g_nearDescendantsByTask[v].push_back(cur.node);

                if (cur.depth == MAX_DEPTH) continue;

                for (int s : succ[cur.node]) {
                    stack.push_back({ s, cur.depth + 1 });
                }
            }
        }

        g_nearDescCacheTaskSig = sig;
        g_nearDescCacheTaskCount = N;
    }

    inline double futureBranchFitRaw(
        const Instance& I,
        int taskIx,
        const Task& t,
        const Resource& r)
    {
        if (taskIx < 0) {
            auto it = I.idToIndex.find(t.id);
            if (it == I.idToIndex.end()) return 0.0;
            taskIx = it->second;
        }

        ensureNearDescendantCache(I);

        if (taskIx < 0 || taskIx >= (int)g_nearDescendantsByTask.size()) {
            return 0.0;
        }

        const auto& desc = g_nearDescendantsByTask[taskIx];
        if (desc.empty()) {
            return 0.0;
        }

        const auto& S = gp::getFeatureScaling();

        double totalWeight = 0.0;
        double coveredWeight = 0.0;

        for (int uIx : desc) {
            if (uIx < 0 || uIx >= (int)I.tasks.size()) continue;

            const Task& u = I.tasks[uIx];
            if (u.start != -1) continue;

            double critNorm = 0.0;
            double slackNorm = 0.0;
            double descNorm = 0.0;
            double durNorm = normalize((double)u.duration, S.maxDuration);

            if (auto cpm = gp::getCPMPrecalc()) {
                if (uIx < (int)cpm->critLen.size() && S.maxCritLen > 0.0) {
                    critNorm = std::min(1.0, (double)cpm->critLen[uIx] / S.maxCritLen);
                }
                if (uIx < (int)cpm->slack.size() && S.maxSlackPos > 0.0) {
                    slackNorm = std::min(
                        1.0,
                        std::max(0.0, (double)cpm->slack[uIx]) / S.maxSlackPos
                    );
                }
                if (uIx < (int)cpm->descCount.size() && S.maxDescCount > 0.0) {
                    descNorm = std::min(1.0, (double)cpm->descCount[uIx] / S.maxDescCount);
                }
            }

            const double weight =
                (1.0 + 1.60 * critNorm + 0.55 * durNorm + 0.20 * descNorm)
                / (1.0 + 2.00 * slackNorm);

            totalWeight += weight;

            if (!u.canBeDoneBy(r)) continue;

            const int reqU = std::max(1, totalReq(u));
            const int lvlU = std::max(0, matchedLevelCached(I, uIx, u, r));
            const double overkill =
                std::max(0.0, (double)(lvlU - reqU) / (double)reqU);

            const double fitQuality = 1.0 / (1.0 + overkill);

            coveredWeight += weight * fitQuality;
        }

        if (totalWeight <= 1e-12) return 0.0;

        double branchFit = coveredWeight / totalWeight;

        const double currentGap = fitGapRatioRaw(I, taskIx, t, r);
        const double currentFit =
            std::isfinite(currentGap) ? (1.0 / (1.0 + currentGap)) : 0.0;

        const double finalValue =
            branchFit * (0.75 + 0.25 * currentFit);

        return clamp01(finalValue);
    }

    inline double futureBranchFitCached(
        const Instance& I,
        int taskIx,
        const Task& t,
        const Resource& r
    ) {
        if (!g_needResFutureBranchFit) {
            return 0.0;
        }

        double cachedValue = 0.0;
        if (tryGetFutureBranchFitStepCached(taskIx, r, cachedValue)) {
            return cachedValue;
        }

        return futureBranchFitRaw(I, taskIx, t, r);
    }

} // namespace

void setOptionalTaskFeatureUsage(
    bool needTaskStructureFeatures,
    bool needTaskResCount,
    bool needTaskAvgResCost,
    bool needTaskUnschedTasks,
    bool needTaskAvailabilityFeatures,
    bool needTaskCostNowFeatures,
    bool needTaskReleasePressure,
    bool needTaskCriticalPressure)
{
    g_needTaskStructureFeatures = needTaskStructureFeatures;
    g_needTaskResCount = needTaskResCount;
    g_needTaskAvgResCost = needTaskAvgResCost;
    g_needTaskUnschedTasks = needTaskUnschedTasks;
    g_needTaskAvailabilityFeatures = needTaskAvailabilityFeatures;
    g_needTaskCostNowFeatures = needTaskCostNowFeatures;
    g_needTaskReleasePressure = needTaskReleasePressure;
    g_needTaskCriticalPressure = needTaskCriticalPressure;
}

void setOptionalResourceFeatureUsage(
    bool needFutureBranchFit,
    bool needBottleneckPreservation,
    bool needSpecialistMisuse)
{
    g_needResFutureBranchFit = needFutureBranchFit;
    g_needResBottleneckPreservation = needBottleneckPreservation;
    g_needResSpecialistMisuse = needSpecialistMisuse;
}

void setFeaturePrecomputed(
    const std::vector<double>* taskResCountByTask,
    const std::vector<double>* avgResCostByTask,
    const std::unordered_map<int, int>* resIndexById,
    const int* unschedCountPtr,
    const std::vector<int>* remainingPredCountByTask,
    const std::vector<int>* latestPredFinishByTask,
    const std::unordered_map<std::string, SkillStepInfo>* skillStepCache,
    const std::vector<int>* matchedLevelByTaskRes,
    int matchedLevelResCount)
{
    g_taskResCountByTask = taskResCountByTask;
    g_avgResCostByTask = avgResCostByTask;
    g_resIndexById = resIndexById;
    g_unschedCountPtr = unschedCountPtr;
    g_remainingPredCountByTask = remainingPredCountByTask;
    g_latestPredFinishByTask = latestPredFinishByTask;
    g_skillStepCache = skillStepCache;
    g_matchedLevelByTaskRes = matchedLevelByTaskRes;
    g_matchedLevelResCount = matchedLevelResCount;
}


void clearFeaturePrecomputed() {
    g_taskResCountByTask = nullptr;
    g_avgResCostByTask = nullptr;
    g_resIndexById = nullptr;
    g_unschedCountPtr = nullptr;
    g_remainingPredCountByTask = nullptr;
    g_latestPredFinishByTask = nullptr;
    g_skillStepCache = nullptr;
    g_matchedLevelByTaskRes = nullptr;
    g_matchedLevelResCount = 0;
}

void buildTaskEvalStepPrecomputed(
    const Instance& I,
    int now,
    const std::vector<int>& readyTaskIdx)
{
    const size_t taskCount = I.tasks.size();

    if (g_taskStepFeatures.size() != taskCount) {
        g_taskStepFeatures.resize(taskCount);
    }

    if (g_taskStepStamp.size() != taskCount) {
        g_taskStepStamp.assign(taskCount, 0);
    }

    nextStamp(g_taskStepCurrentStamp);
    g_taskEvalStepReady = true;

    PriorityContext ctx;
    ctx.inst = &I;
    ctx.now = now;

    for (int ix : readyTaskIdx) {
        if (ix < 0 || ix >= (int)I.tasks.size()) continue;
        g_taskStepFeatures[ix] = computeFeatures(ctx, ix);
        g_taskStepStamp[ix] = g_taskStepCurrentStamp;
    }
}

void clearTaskEvalStepPrecomputed() {
    g_taskEvalStepReady = false;
}

Features computeFeaturesFast(const PriorityContext& ctx, int taskIx) {
    if (g_taskEvalStepReady &&
        taskIx >= 0 &&
        taskIx < (int)g_taskStepFeatures.size() &&
        taskIx < (int)g_taskStepStamp.size() &&
        g_taskStepStamp[taskIx] == g_taskStepCurrentStamp) {
        return g_taskStepFeatures[taskIx];
    }

    return computeFeatures(ctx, taskIx);
}

void buildPairEvalStepPrecomputed(
    const Instance& I,
    int now,
    const std::vector<int>& readyTaskIdx
) {
    const size_t taskCount = I.tasks.size();
    const size_t resCount = I.resources.size();

    if (g_pairBaseTaskFeatures.size() != taskCount) {
        g_pairBaseTaskFeatures.resize(taskCount);
    }

    if (g_pairBaseTaskStamp.size() != taskCount) {
        g_pairBaseTaskStamp.assign(taskCount, 0);
    }

    if (g_pairBaseResourceFeatures.size() != resCount) {
        g_pairBaseResourceFeatures.resize(resCount);
    }

    for (int ri = 0; ri < (int)resCount; ++ri) {
        g_pairBaseResourceFeatures[ri] =
            computeResourceStepBaseRaw(I.resources[ri], now);
    }

    if (g_needResFutureBranchFit) {
        g_pairFutureBranchFitResCount = (int)resCount;

        const size_t pairCount = taskCount * resCount;

        if (g_pairFutureBranchFitCache.size() != pairCount) {
            g_pairFutureBranchFitCache.resize(pairCount);
        }
        if (g_pairFutureBranchFitStamp.size() != pairCount) {
            g_pairFutureBranchFitStamp.assign(pairCount, 0);
        }
    }
    else {
        g_pairFutureBranchFitResCount = 0;
    }

    nextStamp(g_pairCurrentStamp);
    g_pairEvalStepReady = true;

    PriorityContext ctx;
    ctx.inst = &I;
    ctx.now = now;

    for (int ix : readyTaskIdx) {
        if (ix < 0 || ix >= (int)I.tasks.size()) continue;

        const Task& t = I.tasks[ix];
        g_pairBaseTaskFeatures[ix] = computeFeaturesFast(ctx, ix);
        g_pairBaseTaskStamp[ix] = g_pairCurrentStamp;

        if (g_needResFutureBranchFit) {
            auto cacheForResIndex = [&](int ri) {
                if (ri < 0 || ri >= (int)I.resources.size()) return;

                const Resource& rr = I.resources[ri];
                const size_t flatIx =
                    (size_t)ix * (size_t)g_pairFutureBranchFitResCount + (size_t)ri;

                g_pairFutureBranchFitCache[flatIx] =
                    futureBranchFitRaw(I, ix, t, rr);

                g_pairFutureBranchFitStamp[flatIx] = g_pairCurrentStamp;
                };

            if (!t.capableResourceIndices.empty()) {
                for (int ri : t.capableResourceIndices) {
                    cacheForResIndex(ri);
                }
            }
            else {
                for (int ri = 0; ri < (int)I.resources.size(); ++ri) {
                    cacheForResIndex(ri);
                }
            }
        }
    }
}

void clearPairEvalStepPrecomputed() {
    g_pairFutureBranchFitResCount = 0;
    g_pairEvalStepReady = false;
}

Features computeFeatures(const PriorityContext& ctx, int taskIx) {
    Features f{};
    const Instance& I = *ctx.inst;
    const Task& t = I.tasks[taskIx];

    f.duration = t.duration;
    f.reqLevel = (double)t.totalRequiredLevel();
    const int req = totalReq(t);

    const auto& S = gp::getFeatureScaling();

    const bool needStructure =
        g_needTaskStructureFeatures ||
        g_needTaskReleasePressure ||
        g_needTaskCriticalPressure;

    const bool needResCount =
        g_needTaskResCount ||
        g_needTaskReleasePressure ||
        g_needTaskCriticalPressure;

    const bool needAvailability =
        g_needTaskAvailabilityFeatures ||
        g_needTaskCostNowFeatures;

    if (needResCount) {
        f.taskResCount = taskResCountFeature(I, taskIx, t, req);
    }

    if (g_needTaskAvgResCost) {
        f.avgResCostForSkill = avgSalaryForSkill(I, taskIx, t, req);
    }

    if (g_needTaskUnschedTasks) {
        f.unschedTasks = (double)countUnschedTasks(I);
    }

    if (needAvailability) {
        bool predsDone = false;
        if (g_remainingPredCountByTask &&
            taskIx >= 0 &&
            taskIx < (int)g_remainingPredCountByTask->size()) {
            predsDone = ((*g_remainingPredCountByTask)[taskIx] == 0);
        }
        else {
            predsDone = predecessorsDoneNow(I, t, ctx.now);
        }

        double availCached = -1.0;

        if (req > 0 && canUseSingleSkillCache(t) && !t.reqSkill.empty()) {
            availCached = cachedAvailSkill(t.reqSkill);
        }

        f.availSkill = (availCached >= 0.0)
            ? availCached
            : bestFreeMatchedLevel(I, taskIx, t, ctx.now);

        f.availGap = f.availSkill - (double)req;
        f.feasibleNow = predsDone && (f.availSkill >= (double)req);
    }

    if (needStructure) {
        if (auto cpm = gp::getCPMPrecalc()) {
            if (taskIx >= 0 && taskIx < (int)cpm->critLen.size()) {
                f.critLen = cpm->critLen[taskIx];
            }
            if (taskIx >= 0 && taskIx < (int)cpm->slack.size()) {
                f.slack = cpm->slack[taskIx];
            }
            if (taskIx >= 0 && taskIx < (int)cpm->descCount.size()) {
                f.descCount = cpm->descCount[taskIx];
            }
        }
    }

    if (g_needTaskReleasePressure) {
        f.taskReleasePressure = taskReleasePressureRaw(
            t,
            f.taskResCount,
            f.critLen,
            f.slack,
            f.descCount
        );
    }

    if (g_needTaskCostNowFeatures) {
        fillCostNowFeatures(f, I, t, req, ctx.now);
    }

    normalizeTaskFeatures(f, S);

    if (g_needTaskCriticalPressure) {
        const double structuralUrgency = clamp01(
            0.55 * f.critLen +
            0.30 * (1.0 - f.slack) +
            0.15 * f.descCount
        );

        const double scarcityPressure = clamp01(
            0.55 * (1.0 - f.taskResCount) +
            0.45 * f.reqLevel
        );

        f.taskCriticalPressure = clamp01(
            0.72 * structuralUrgency +
            0.28 * scarcityPressure
        );
    }

    return f;
}


Features computeResourceFeatures(const Instance& I, const Task& t, const Resource& r, int now) {
    Features f{};

    ResourceStepBase base{};
    if (tryGetResourceStepBaseCached(r, base)) {
        assignResourceStepBaseToFeatures(base, f);
    }
    else {
        base = computeResourceStepBaseRaw(r, now);
        assignResourceStepBaseToFeatures(base, f);
    }

    const bool canStartNowRaw = (f.resCanStartNow > 0.5);

    const int taskIx =
        (I.idToIndex.count(t.id) ? I.idToIndex.at(t.id) : -1);

    const int req = totalReq(t);
    const int lvlNow = matchedLevelCached(I, taskIx, t, r);

    f.resSkillLevel = (double)lvlNow;
    f.resWagePerLevel = r.salary / (double)std::max(1, lvlNow);
    f.resAssignCost = r.salary * (double)t.duration;

    double cheapestNow = std::numeric_limits<double>::infinity();
    double cheapestCapableOverall = std::numeric_limits<double>::infinity();

    for (const auto& rr : I.resources) {
        int lvl = 0;
        auto it = rr.skills.find(t.reqSkill);
        if (it != rr.skills.end()) lvl = it->second;

        if (req > 0 && lvl < req) continue;

        if (rr.salary < cheapestCapableOverall) {
            cheapestCapableOverall = rr.salary;
        }

        if (rr.busyUntil <= now) {
            cheapestNow = std::min(cheapestNow, rr.salary);
        }
    }

    f.resRelativeWage =
        std::isfinite(cheapestNow) ? (r.salary - cheapestNow) : 0.0;

    f.resAssignPremiumAll =
        std::isfinite(cheapestCapableOverall)
        ? std::max(0.0, r.salary - cheapestCapableOverall) * (double)t.duration
        : 0.0;

    const auto& S = gp::getFeatureScaling();
    double reservePressure = 0.0;
    double criticalReserve = 0.0;
    std::unordered_map<std::string, double> familyPressure;

    for (int uIx = 0; uIx < (int)I.tasks.size(); ++uIx) {
        const auto& u = I.tasks[uIx];
        if (u.start != -1) continue;
        if (u.id == t.id) continue;

        int reqU = totalReq(u);
        if (reqU > 0 && !u.canBeDoneBy(r)) continue;

        int feasibleCount = 0;
        double cheapest = std::numeric_limits<double>::infinity();
        double second = std::numeric_limits<double>::infinity();

        for (const auto& rr : I.resources) {
            if (reqU > 0 && !u.canBeDoneBy(rr)) continue;

            ++feasibleCount;

            if (rr.salary < cheapest) {
                second = cheapest;
                cheapest = rr.salary;
            }
            else if (rr.salary < second) {
                second = rr.salary;
            }
        }

        if (!std::isfinite(second)) second = cheapest;
        const double priceGap = std::max(0.0, second - cheapest);

        const double reserveContribution =
            ((double)u.duration * priceGap) / (double)std::max(1, feasibleCount);

        reservePressure += reserveContribution;

        double critRaw = 0.0;
        double slackRaw = 0.0;
        double descRaw = 0.0;

        if (auto cpm = gp::getCPMPrecalc()) {
            if (uIx >= 0 && uIx < (int)cpm->critLen.size())   critRaw = cpm->critLen[uIx];
            if (uIx >= 0 && uIx < (int)cpm->slack.size())     slackRaw = cpm->slack[uIx];
            if (uIx >= 0 && uIx < (int)cpm->descCount.size()) descRaw = cpm->descCount[uIx];
        }

        const double critNorm =
            (S.maxCritLen > 0.0) ? (critRaw / S.maxCritLen) : 0.0;
        const double slackNorm =
            (S.maxSlackPos > 0.0) ? (std::max(0.0, slackRaw) / S.maxSlackPos) : 0.0;
        const double descNorm =
            (S.maxNumTasks > 0.0) ? (descRaw / S.maxNumTasks) : 0.0;

        const double structuralPressure =
            (1.0 + critNorm + descNorm) / (1.0 + slackNorm);

        criticalReserve += reserveContribution * structuralPressure;

        const std::string famKey = u.requirementKey();
        familyPressure[famKey] += reserveContribution;
    }

    f.resReservePressure = reservePressure;

    if (g_needResFutureBranchFit) {
        f.resFutureBranchFit = futureBranchFitCached(I, taskIx, t, r);
    }

    if (g_needResBottleneckPreservation) {
        f.resBottleneckPreservation =
            bottleneckPreservationRaw(I, taskIx, t, criticalReserve);
    }

    if (g_needResSpecialistMisuse) {
        f.resSpecialistMisuse =
            specialistMisuseRaw(I, taskIx, t, r, criticalReserve);
    }

    const std::string currentFamKey = t.requirementKey();

    double currentFamilyPressure = 0.0;
    auto itFam = familyPressure.find(currentFamKey);
    if (itFam != familyPressure.end()) {
        currentFamilyPressure = itFam->second;
    }

    double bestOtherFamilyPressure = 0.0;
    for (const auto& kv : familyPressure) {
        if (kv.first == currentFamKey) continue;
        if (kv.second > bestOtherFamilyPressure) {
            bestOtherFamilyPressure = kv.second;
        }
    }

    f.resFamilyMismatch =
        bestOtherFamilyPressure / (1.0 + currentFamilyPressure);

    f.resWage = normalize(f.resWage, S.maxMinWageAvail);
    f.resSkillLevel = normalize(f.resSkillLevel, S.maxResSkillLevel);
    f.resIdleTime = normalize(f.resIdleTime, S.maxWaitRes);
    f.resCanStartNow = canStartNowRaw ? 1.0 : 0.0;
    f.resUtilization = normalize(f.resUtilization, 1.0);
    f.resWagePerLevel = normalize(f.resWagePerLevel, S.maxResWagePerLevel);
    f.resAssignCost = normalize(f.resAssignCost, S.maxMinWageAvail * S.maxDuration);
    f.resAssignPremiumAll = normalize(f.resAssignPremiumAll, S.maxMinWageAvail * S.maxDuration);
    f.resFamilyMismatch = normalize(f.resFamilyMismatch, S.maxResReservePressure);
    f.resFutureBranchFit = clamp01(f.resFutureBranchFit);
    f.resBottleneckPreservation = normalize(
        f.resBottleneckPreservation,
        S.maxResBottleneckPreservation
    );
    f.resSpecialistMisuse = normalize(
        f.resSpecialistMisuse,
        S.maxResSpecialistMisuse
    );
    f.resRelativeWage = normalize(f.resRelativeWage, S.maxResRelativeWage);
    f.resReservePressure = normalize(f.resReservePressure, S.maxResReservePressure);

    return f;
}


Features computeResourceFeaturesFast(
    const Instance& I,
    int taskIx,
    const Task& t,
    const Resource& r,
    int now,
    double cheapestNow,
    double cheapestCapableOverall,
    double waitOfCheapestCapableOverall,
    double reservePressureExcludingTask,
    double criticalReserveExcludingTask,
    double familyMismatchExcludingTask
) {
    (void)waitOfCheapestCapableOverall;

    Features f{};

    ResourceStepBase base{};
    if (tryGetResourceStepBaseCached(r, base)) {
        assignResourceStepBaseToFeatures(base, f);
    }
    else {
        base = computeResourceStepBaseRaw(r, now);
        assignResourceStepBaseToFeatures(base, f);
    }

    const bool canStartNowRaw = (f.resCanStartNow > 0.5);

    const int req = totalReq(t);
    const int lvlNow = matchedLevelCached(I, taskIx, t, r);

    f.resSkillLevel = (double)lvlNow;
    f.resWagePerLevel = r.salary / (double)std::max(1, lvlNow);
    f.resAssignCost = r.salary * (double)t.duration;

    f.resRelativeWage =
        std::isfinite(cheapestNow) ? (r.salary - cheapestNow) : 0.0;

    f.resAssignPremiumAll =
        std::isfinite(cheapestCapableOverall)
        ? std::max(0.0, r.salary - cheapestCapableOverall) * (double)t.duration
        : 0.0;

    f.resReservePressure = reservePressureExcludingTask;
    f.resFamilyMismatch = familyMismatchExcludingTask;

    if (g_needResFutureBranchFit) {
        f.resFutureBranchFit = futureBranchFitCached(I, taskIx, t, r);
    }

    if (g_needResBottleneckPreservation) {
        f.resBottleneckPreservation =
            bottleneckPreservationRaw(I, taskIx, t, criticalReserveExcludingTask);
    }

    if (g_needResSpecialistMisuse) {
        f.resSpecialistMisuse =
            specialistMisuseRaw(I, taskIx, t, r, criticalReserveExcludingTask);
    }

    const auto& S = gp::getFeatureScaling();

    f.resWage = normalize(f.resWage, S.maxMinWageAvail);
    f.resSkillLevel = normalize(f.resSkillLevel, S.maxResSkillLevel);
    f.resIdleTime = normalize(f.resIdleTime, S.maxWaitRes);
    f.resCanStartNow = canStartNowRaw ? 1.0 : 0.0;
    f.resUtilization = normalize(f.resUtilization, 1.0);
    f.resWagePerLevel = normalize(f.resWagePerLevel, S.maxResWagePerLevel);
    f.resAssignCost = normalize(f.resAssignCost, S.maxMinWageAvail * S.maxDuration);
    f.resAssignPremiumAll = normalize(f.resAssignPremiumAll, S.maxMinWageAvail * S.maxDuration);
    f.resFamilyMismatch = normalize(f.resFamilyMismatch, S.maxResReservePressure);
    f.resFutureBranchFit = clamp01(f.resFutureBranchFit);
    f.resBottleneckPreservation = normalize(
        f.resBottleneckPreservation,
        S.maxResBottleneckPreservation
    );
    f.resSpecialistMisuse = normalize(
        f.resSpecialistMisuse,
        S.maxResSpecialistMisuse
    );
    f.resRelativeWage = normalize(f.resRelativeWage, S.maxResRelativeWage);
    f.resReservePressure = normalize(f.resReservePressure, S.maxResReservePressure);

    return f;
}

Features computePairFeaturesFast(
    const Instance& I,
    int taskIx,
    const Task& t,
    const Resource& r,
    int now,
    double cheapestNow,
    double cheapestCapableOverall,
    double waitOfCheapestCapableOverall,
    double reservePressureExcludingTask,
    double criticalReserveExcludingTask,
    double familyMismatchExcludingTask
) {
    Features f{};

    if (g_pairEvalStepReady &&
        taskIx >= 0 &&
        taskIx < (int)g_pairBaseTaskFeatures.size() &&
        taskIx < (int)g_pairBaseTaskStamp.size() &&
        g_pairBaseTaskStamp[taskIx] == g_pairCurrentStamp) {
        f = g_pairBaseTaskFeatures[taskIx];
    }
    else {
        PriorityContext ctx;
        ctx.inst = &I;
        ctx.now = now;
        f = computeFeatures(ctx, taskIx);
    }

    Features rf = computeResourceFeaturesFast(
        I,
        taskIx,
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

    f.resWage = rf.resWage;
    f.resSkillLevel = rf.resSkillLevel;
    f.resIdleTime = rf.resIdleTime;
    f.resCanStartNow = rf.resCanStartNow;
    f.resUtilization = rf.resUtilization;
    f.resWagePerLevel = rf.resWagePerLevel;
    f.resAssignCost = rf.resAssignCost;
    f.resAssignPremiumAll = rf.resAssignPremiumAll;
    f.resReservePressure = rf.resReservePressure;
    f.resFamilyMismatch = rf.resFamilyMismatch;
    f.resFutureBranchFit = rf.resFutureBranchFit;
    f.resBottleneckPreservation = rf.resBottleneckPreservation;
    f.resSpecialistMisuse = rf.resSpecialistMisuse;
    f.resRelativeWage = rf.resRelativeWage;

    return f;
}
