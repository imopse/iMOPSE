#include "Features.hpp"
#include "FeatureScaling.hpp"
#include <algorithm>
#include <cstdint>
#include <limits>
#include <cmath>
#include "Precompute.hpp"

namespace gphh_so {

    namespace {

        const std::unordered_map<int, int>* g_resIndexById = nullptr;
        const std::vector<int>* g_remainingPredCountByTask = nullptr;
        const std::vector<int>* g_latestPredFinishByTask = nullptr;

        std::vector<Features> g_taskStepFeatures;
        std::vector<std::uint64_t> g_taskStepStamp;
        std::uint64_t g_taskStepCurrentStamp = 1;
        bool g_taskEvalStepReady = false;

        std::vector<Features> g_pairBaseTaskFeatures;
        std::vector<std::uint64_t> g_pairBaseTaskStamp;
        std::uint64_t g_pairCurrentStamp = 1;
        bool g_pairEvalStepReady = false;
        std::vector<double> g_rootEarlyBaseByTask;
        std::vector<double> g_resourceReserveScoreByRes;
        std::vector<double> g_minReserveScoreByTask;
        std::vector<double> g_maxReserveLossByTask;
        std::vector<double> g_minFreeSalaryByTask;
        std::vector<double> g_maxFreeExtraCostByTask;
        std::vector<std::uint64_t> g_freeCostStepStamp;

        int g_cachedTaskCount = 0;
        int g_cachedResCount = 0;
        std::size_t g_staticCacheSignature = 0;
        bool g_staticCacheReady = false;
        inline std::uint64_t nextStamp(std::uint64_t& current) {
            ++current;
            if (current == 0) current = 1;
            return current;
        }

        inline double normalize01(double value, double maxValue) {
            if (!std::isfinite(value) || value <= 0.0) return 0.0;
            if (!std::isfinite(maxValue) || maxValue <= 0.0) return 0.0;

            double x = value / maxValue;
            if (x < 0.0) x = 0.0;
            if (x > 1.0) x = 1.0;
            return x;
        }

        static std::size_t localResourceSignature(const Instance& I) {
            std::size_t sig = Instance::hashCombine(1469598103934665603ull, I.resources.size());

            for (const auto& r : I.resources) {
                std::size_t resHash = Instance::hashCombine(std::hash<int>{}(r.id), r.skills.size());
                resHash = Instance::hashCombine(resHash, std::hash<long long>{}((long long)(r.salary * 1000.0)));

                for (const auto& kv : r.skills) {
                    std::size_t pairHash = std::hash<std::string>{}(kv.first);
                    pairHash = Instance::hashCombine(pairHash, std::hash<int>{}(kv.second));
                    resHash = Instance::hashCombine(resHash, pairHash);
                }

                sig = Instance::hashCombine(sig, resHash);
            }

            return sig;
        }

        static std::size_t localTaskSignature(const Instance& I) {
            std::size_t sig = Instance::hashCombine(1099511628211ull, I.tasks.size());

            for (const auto& t : I.tasks) {
                std::size_t taskHash = Instance::hashCombine(std::hash<int>{}(t.id), std::hash<int>{}(t.duration));
                taskHash = Instance::hashCombine(taskHash, std::hash<std::string>{}(t.requirementKey()));
                taskHash = Instance::hashCombine(taskHash, std::hash<int>{}(t.totalRequiredLevel()));

                for (int pid : t.predecessors) {
                    taskHash = Instance::hashCombine(taskHash, std::hash<int>{}(pid));
                }

                sig = Instance::hashCombine(sig, taskHash);
            }

            return sig;
        }

        static std::size_t featureCacheSignature(const Instance& I) {
            const std::size_t rsig =
                I.resourceStructureSignatureReady ? I.resourceStructureSignature : localResourceSignature(I);
            const std::size_t tsig =
                I.taskStructureSignatureReady ? I.taskStructureSignature : localTaskSignature(I);

            std::size_t sig = Instance::hashCombine(tsig, rsig);
            sig = Instance::hashCombine(sig, I.tasks.size());
            sig = Instance::hashCombine(sig, I.resources.size());
            return sig;
        }

        inline int resourceIndexCached(const Instance& I, const Resource& r) {
            if (g_resIndexById) {
                auto it = g_resIndexById->find(r.id);
                if (it != g_resIndexById->end()) return it->second;
            }

            for (int i = 0; i < (int)I.resources.size(); ++i) {
                if (I.resources[i].id == r.id) return i;
            }

            return -1;
        }

        inline bool predecessorsDoneNow(const Instance& I, int taskIx, const Task& t, int now) {
            if (g_remainingPredCountByTask &&
                taskIx >= 0 &&
                taskIx < (int)g_remainingPredCountByTask->size()) {
                return (*g_remainingPredCountByTask)[taskIx] == 0;
            }

            for (int pid : t.predecessors) {
                auto it = I.idToIndex.find(pid);
                if (it != I.idToIndex.end()) {
                    const Task& p = I.tasks[it->second];
                    if (p.finish < 0 || p.finish > now) return false;
                }
            }

            return true;
        }

        inline bool hasFreeCapableResourceNow(const Instance& I, const Task& t, int now) {
            if (!t.capableResourceIndices.empty()) {
                for (int ri : t.capableResourceIndices) {
                    if (ri < 0 || ri >= (int)I.resources.size()) continue;
                    if (I.resources[ri].busyUntil <= now) return true;
                }
                return false;
            }

            for (const auto& r : I.resources) {
                if (r.busyUntil > now) continue;
                if (t.canBeDoneBy(r)) return true;
            }

            return false;
        }

        static double taskUnlockBucketRaw(const Instance& I, const gp::CPMPrecalc* cpm, int taskIx) {
            if (!cpm) return 0.0;
            if (taskIx < 0 || taskIx >= (int)I.tasks.size()) return 0.0;
            if (taskIx >= (int)cpm->critLen.size()) return 0.0;
            if (cpm->cmaxCPM <= 0) return 0.0;

            const Task& t = I.tasks[taskIx];
            const double downstream = std::max(0.0, (double)cpm->critLen[taskIx] - (double)t.duration);
            const double denom = std::max(1.0, (double)cpm->cmaxCPM);

            double x = downstream / denom;
            if (x < 0.0) x = 0.0;
            if (x > 1.0) x = 1.0;
            return x;
        }

        static double taskScarcityBucketRaw(const Instance& I, const Task& t) {
            int n = 0;

            if (!t.capableResourceIndices.empty()) {
                n = (int)t.capableResourceIndices.size();
            }
            else if (!t.capableResources.empty()) {
                n = (int)t.capableResources.size();
            }
            else {
                for (const auto& r : I.resources) {
                    if (t.canBeDoneBy(r)) ++n;
                }
            }

            if (n <= 2) return 2.0;
            if (n <= 4) return 1.0;
            return 0.0;
        }

        static double taskLongFlagRaw(const Instance& I, int taskIx) {
            if (taskIx < 0 || taskIx >= g_cachedTaskCount) return 0.0;
            return g_rootEarlyBaseByTask[taskIx];
        }

        static double taskReadyAgeRaw(const Instance& I, const Task& t, int taskIx, int now) {
            (void)I;
            (void)t;
            (void)now;


            const gp::CPMPrecalc* cpm = gp::getCPMPrecalc();
            if (!cpm) return 0.0;
            if (taskIx < 0 || taskIx >= (int)cpm->slack.size()) return 0.0;

            int slack = cpm->slack[taskIx];
            if (slack <= 0) return 1.0;

            return 1.0 / (1.0 + (double)slack);
        }

        static void ensureStaticFiveFeatureCaches(const Instance& I) {
            const std::size_t sig = featureCacheSignature(I);
            const int taskCount = (int)I.tasks.size();
            const int resCount = (int)I.resources.size();

            if (g_staticCacheReady &&
                g_staticCacheSignature == sig &&
                g_cachedTaskCount == taskCount &&
                g_cachedResCount == resCount) {
                return;
            }

            g_resourceReserveScoreByRes.assign(resCount, 0.0);
            g_minReserveScoreByTask.assign(taskCount, 0.0);
            g_maxReserveLossByTask.assign(taskCount, 1.0);
            g_rootEarlyBaseByTask.assign(taskCount, 0.0);

            g_cachedTaskCount = taskCount;
            g_cachedResCount = resCount;

            const gp::CPMPrecalc* cpm = gp::getCPMPrecalc();
            const double cmax = (cpm && cpm->cmaxCPM > 0) ? (double)cpm->cmaxCPM : 1.0;

            for (int ti = 0; ti < taskCount; ++ti) {
                const Task& t = I.tasks[ti];

                int capableCount = 0;
                if (!t.capableResourceIndices.empty()) {
                    capableCount = (int)t.capableResourceIndices.size();
                }
                else {
                    for (const auto& r : I.resources) {
                        if (t.canBeDoneBy(r)) ++capableCount;
                    }
                }

                if (capableCount <= 0) continue;

                double critNorm = 0.0;
                if (cpm && ti < (int)cpm->critLen.size()) {
                    critNorm = std::max(0.0, (double)cpm->critLen[ti]) / cmax;
                    if (critNorm > 1.0) critNorm = 1.0;
                }

                const double importance = (1.0 + critNorm) / (double)capableCount;

                if (!t.capableResourceIndices.empty()) {
                    for (int ri : t.capableResourceIndices) {
                        if (ri < 0 || ri >= resCount) continue;
                        g_resourceReserveScoreByRes[ri] += importance;
                    }
                }
                else {
                    for (int ri = 0; ri < resCount; ++ri) {
                        if (t.canBeDoneBy(I.resources[ri])) {
                            g_resourceReserveScoreByRes[ri] += importance;
                        }
                    }
                }
            }

            for (int ti = 0; ti < taskCount; ++ti) {
                const Task& t = I.tasks[ti];

                double minReserve = std::numeric_limits<double>::infinity();
                double maxReserve = 0.0;

                if (!t.capableResourceIndices.empty()) {
                    for (int ri : t.capableResourceIndices) {
                        if (ri < 0 || ri >= resCount) continue;
                        const double s = g_resourceReserveScoreByRes[ri];
                        minReserve = std::min(minReserve, s);
                        maxReserve = std::max(maxReserve, s);
                    }
                }
                else {
                    for (int ri = 0; ri < resCount; ++ri) {
                        if (!t.canBeDoneBy(I.resources[ri])) continue;
                        const double s = g_resourceReserveScoreByRes[ri];
                        minReserve = std::min(minReserve, s);
                        maxReserve = std::max(maxReserve, s);
                    }
                }

                if (!std::isfinite(minReserve)) {
                    minReserve = 0.0;
                    maxReserve = 0.0;
                }

                g_minReserveScoreByTask[ti] = minReserve;

                const double maxLoss = maxReserve - minReserve;
                g_maxReserveLossByTask[ti] = (maxLoss > 0.0 && std::isfinite(maxLoss))
                    ? maxLoss
                    : 1.0;
            }

            double maxRootDuration = 1.0;
            for (int ti = 0; ti < taskCount; ++ti) {
                const Task& t = I.tasks[ti];
                if (t.predecessors.empty()) {
                    maxRootDuration = std::max(maxRootDuration, (double)t.duration);
                }
            }

            for (int ti = 0; ti < taskCount; ++ti) {
                const Task& t = I.tasks[ti];

                if (!t.predecessors.empty()) {
                    g_rootEarlyBaseByTask[ti] = 0.0;
                    continue;
                }

                const double durNorm = std::max(0.0, (double)t.duration) / maxRootDuration;

                const double downstream = taskUnlockBucketRaw(I, cpm, ti);

                const double scarcityNorm = 0.5 * taskScarcityBucketRaw(I, t);

                const bool hasSuccessor =
                    (cpm && ti < (int)cpm->descCount.size() && cpm->descCount[ti] > 0);

                double core = 0.0;
                if (hasSuccessor) {
                    core = std::max(durNorm, std::min(1.0, 0.35 + downstream));
                }
                else {
                    core = 0.75 * durNorm;
                }

                double base = core * (1.0 - 0.35 * scarcityNorm);

                if (base < 0.0) base = 0.0;
                if (base > 1.0) base = 1.0;

                g_rootEarlyBaseByTask[ti] = base;
            }

            g_staticCacheSignature = sig;
            g_staticCacheReady = true;
        }

        inline double cachedFreeExtraCostLoss(const Instance& I, int taskIx, const Resource& r) {
            if (taskIx < 0 || taskIx >= g_cachedTaskCount) return 0.0;
            if (taskIx >= (int)g_freeCostStepStamp.size()) return 0.0;
            if (g_freeCostStepStamp[taskIx] != g_pairCurrentStamp) return 0.0;

            const double minFreeSalary = g_minFreeSalaryByTask[taskIx];
            const double denom = g_maxFreeExtraCostByTask[taskIx];

            if (!(denom > 0.0) || !std::isfinite(denom)) return 0.0;
            if (!(minFreeSalary > 0.0) || !std::isfinite(minFreeSalary)) return 0.0;

            const Task& t = I.tasks[taskIx];
            const double extra =
                (double)t.duration * std::max(0.0, r.salary - minFreeSalary);

            double x = extra / denom;
            if (x < 0.0) x = 0.0;
            if (x > 1.0) x = 1.0;
            return x;
        }

        inline double cachedResCostPremium(const Instance& I, int taskIx, const Resource& r) {
            ensureStaticFiveFeatureCaches(I);

            if (taskIx < 0 || taskIx >= g_cachedTaskCount) return 0.0;

            const int ri = resourceIndexCached(I, r);
            if (ri < 0 || ri >= g_cachedResCount) return 0.0;

            const double reserveScore = g_resourceReserveScoreByRes[ri];
            const double minReserve = g_minReserveScoreByTask[taskIx];
            const double reserveDenom = g_maxReserveLossByTask[taskIx];

            double reserveLoss = 0.0;
            if ((reserveDenom > 0.0) && std::isfinite(reserveDenom)) {
                reserveLoss = (reserveScore - minReserve) / reserveDenom;
                if (reserveLoss < 0.0) reserveLoss = 0.0;
                if (reserveLoss > 1.0) reserveLoss = 1.0;
            }


            const double freeExtraCostLoss = cachedFreeExtraCostLoss(I, taskIx, r);

            return 0.7 * freeExtraCostLoss + 0.3 * reserveLoss;
        }

    }

    void setOptionalTaskFeatureUsage(bool, bool, bool, bool, bool, bool, bool, bool) {}
    void setOptionalResourceFeatureUsage(bool, bool, bool) {}

    void setFeaturePrecomputed(
        const std::vector<double>*,
        const std::vector<double>*,
        const std::unordered_map<int, int>* resIndexById,
        const int*,
        const std::vector<int>* remainingPredCountByTask,
        const std::vector<int>* latestPredFinishByTask,
        const std::unordered_map<std::string, SkillStepInfo>*,
        const std::vector<int>*,
        int)
    {
        g_resIndexById = resIndexById;
        g_remainingPredCountByTask = remainingPredCountByTask;
        g_latestPredFinishByTask = latestPredFinishByTask;
    }

    void clearFeaturePrecomputed() {
        g_resIndexById = nullptr;
        g_remainingPredCountByTask = nullptr;
        g_latestPredFinishByTask = nullptr;
    }

    void buildTaskEvalStepPrecomputed(
        const Instance& I,
        int now,
        const std::vector<int>& readyTaskIdx)
    {
        ensureStaticFiveFeatureCaches(I);

        const size_t taskCount = I.tasks.size();

        if (g_taskStepFeatures.size() != taskCount) {
            g_taskStepFeatures.resize(taskCount);
        }
        if (g_taskStepStamp.size() != taskCount) {
            g_taskStepStamp.assign(taskCount, 0);
        }

        nextStamp(g_taskStepCurrentStamp);
        g_taskEvalStepReady = true;

        PriorityContext ctx{ &I, now };

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
        const std::vector<int>& readyTaskIdx)
    {
        ensureStaticFiveFeatureCaches(I);

        const size_t taskCount = I.tasks.size();

        if (g_pairBaseTaskFeatures.size() != taskCount) {
            g_pairBaseTaskFeatures.resize(taskCount);
        }
        if (g_pairBaseTaskStamp.size() != taskCount) {
            g_pairBaseTaskStamp.assign(taskCount, 0);
        }
        if (g_minFreeSalaryByTask.size() != taskCount) {
            g_minFreeSalaryByTask.assign(taskCount, 0.0);
        }
        if (g_maxFreeExtraCostByTask.size() != taskCount) {
            g_maxFreeExtraCostByTask.assign(taskCount, 1.0);
        }
        if (g_freeCostStepStamp.size() != taskCount) {
            g_freeCostStepStamp.assign(taskCount, 0);
        }

        nextStamp(g_pairCurrentStamp);
        g_pairEvalStepReady = true;

        PriorityContext ctx{ &I, now };

        for (int ix : readyTaskIdx) {
            if (ix < 0 || ix >= (int)I.tasks.size()) continue;

            g_pairBaseTaskFeatures[ix] = computeFeaturesFast(ctx, ix);
            g_pairBaseTaskStamp[ix] = g_pairCurrentStamp;

            const Task& t = I.tasks[ix];

            double minFreeSalary = std::numeric_limits<double>::infinity();
            double maxFreeSalary = 0.0;
            int freeCount = 0;

            if (!t.capableResourceIndices.empty()) {
                for (int ri : t.capableResourceIndices) {
                    if (ri < 0 || ri >= (int)I.resources.size()) continue;
                    const Resource& r = I.resources[ri];
                    if (r.busyUntil > now) continue;

                    ++freeCount;
                    minFreeSalary = std::min(minFreeSalary, r.salary);
                    maxFreeSalary = std::max(maxFreeSalary, r.salary);
                }
            }
            else {
                for (int ri = 0; ri < (int)I.resources.size(); ++ri) {
                    const Resource& r = I.resources[ri];
                    if (r.busyUntil > now) continue;
                    if (!t.canBeDoneBy(r)) continue;

                    ++freeCount;
                    minFreeSalary = std::min(minFreeSalary, r.salary);
                    maxFreeSalary = std::max(maxFreeSalary, r.salary);
                }
            }

            if (freeCount <= 0 || !std::isfinite(minFreeSalary)) {
                g_minFreeSalaryByTask[ix] = 0.0;
                g_maxFreeExtraCostByTask[ix] = 1.0;
                g_freeCostStepStamp[ix] = g_pairCurrentStamp;
                continue;
            }

            g_minFreeSalaryByTask[ix] = minFreeSalary;

            const double maxExtra =
                (double)t.duration * std::max(0.0, maxFreeSalary - minFreeSalary);

            g_maxFreeExtraCostByTask[ix] =
                (maxExtra > 0.0 && std::isfinite(maxExtra)) ? maxExtra : 1.0;

            g_freeCostStepStamp[ix] = g_pairCurrentStamp;
        }
    }

    void clearPairEvalStepPrecomputed() {
        g_pairEvalStepReady = false;
    }

    Features computeFeatures(const PriorityContext& ctx, int taskIx) {
        Features f{};
        const Instance& I = *ctx.inst;
        const Task& t = I.tasks[taskIx];

        const bool predsDone = predecessorsDoneNow(I, taskIx, t, ctx.now);
        const bool hasCapNow = hasFreeCapableResourceNow(I, t, ctx.now);

        f.feasibleNow = predsDone && hasCapNow;
        f.taskUnlockBucket = taskUnlockBucketRaw(I, gp::getCPMPrecalc(), taskIx);
        f.taskScarcityBucket = 0.5 * taskScarcityBucketRaw(I, t);
        ensureStaticFiveFeatureCaches(I);
        f.taskLongFlag = taskLongFlagRaw(I, taskIx);
        f.taskReadyAge = taskReadyAgeRaw(I, t, taskIx, ctx.now);

        return f;
    }

    Features computeResourceFeatures(
        const Instance& I,
        const Task& t,
        const Resource& r,
        int now)
    {
        const auto it = I.idToIndex.find(t.id);
        const int taskIx = (it != I.idToIndex.end()) ? it->second : -1;

        Features f{};
        f.feasibleNow = true;
        f.resCostPremium = cachedResCostPremium(I, taskIx, r);
        f.resWaitIfChosen = normalize01(
            (double)std::max(0, r.busyUntil - now),
            gp::getFeatureScaling().maxWaitRes
        );
        return f;
    }

    Features computeResourceFeaturesFast(
        const Instance& I,
        int taskIx,
        const Task& t,
        const Resource& r,
        int now,
        double,
        double,
        double,
        double reservePressureExcludingTask,
        double criticalReserveExcludingTask,
        double familyMismatchExcludingTask)
    {
        (void)reservePressureExcludingTask;

        const auto& S = gp::getFeatureScaling();

        Features f{};
        f.feasibleNow = true;
        f.resCostPremium = cachedResCostPremium(I, taskIx, r);

        const double waitIfChosenRaw = (double)std::max(0, r.busyUntil - now);
        f.resWaitIfChosen = normalize01(waitIfChosenRaw, S.maxWaitRes);

        f.resScarceFamilyLoad = normalize01(
            criticalReserveExcludingTask,
            S.maxResCriticalReserve
        );

        int feasibleCount = 0;
        if (!t.capableResourceIndices.empty()) {
            feasibleCount = (int)t.capableResourceIndices.size();
        }
        else if (!t.capableResources.empty()) {
            feasibleCount = (int)t.capableResources.size();
        }
        else {
            for (const auto& rr : I.resources) {
                if (t.canBeDoneBy(rr)) ++feasibleCount;
            }
        }
        feasibleCount = std::max(1, feasibleCount);

        const double reqSafe = (double)std::max(1, t.totalRequiredLevel());
        const double replaceability = (double)feasibleCount / reqSafe;

        double critNorm = 0.0;
        double slackNorm = 0.0;
        if (auto cpm = gp::getCPMPrecalc()) {
            if (taskIx >= 0 && taskIx < (int)cpm->critLen.size()) {
                critNorm = normalize01((double)cpm->critLen[taskIx], S.maxCritLen);
            }
            if (taskIx >= 0 && taskIx < (int)cpm->slack.size()) {
                slackNorm = normalize01(
                    std::max(0.0, (double)cpm->slack[taskIx]),
                    S.maxSlackPos
                );
            }
        }

        const double easyFactor = (1.0 + slackNorm) / (1.0 + critNorm);

        const double matched = (double)std::max(0, t.matchedLevelOn(r));
        const double overkill = std::max(0.0, matched - reqSafe) / reqSafe;

        double specialistMisuseRaw =
            criticalReserveExcludingTask
            * replaceability
            * easyFactor
            * (1.0 + overkill);

        if (familyMismatchExcludingTask > 0.0) {
            specialistMisuseRaw *= (1.0 + familyMismatchExcludingTask);
        }

        f.pairSpecialistMisuse = normalize01(
            specialistMisuseRaw,
            S.maxResSpecialistMisuse
        );

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
        double familyMismatchExcludingTask)
    {
        Features f{};

        if (g_pairEvalStepReady &&
            taskIx >= 0 &&
            taskIx < (int)g_pairBaseTaskFeatures.size() &&
            taskIx < (int)g_pairBaseTaskStamp.size() &&
            g_pairBaseTaskStamp[taskIx] == g_pairCurrentStamp) {
            f = g_pairBaseTaskFeatures[taskIx];
        }
        else {
            PriorityContext ctx{ &I, now };
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

        f.resCostPremium = rf.resCostPremium;
        f.resScarceFamilyLoad = rf.resScarceFamilyLoad;
        f.resWaitIfChosen = rf.resWaitIfChosen;
        f.pairSpecialistMisuse = rf.pairSpecialistMisuse;
        f.taskLongFlag = f.taskLongFlag * (1.0 - 0.65 * rf.resCostPremium);

        if (f.taskLongFlag < 0.0) f.taskLongFlag = 0.0;
        if (f.taskLongFlag > 1.0) f.taskLongFlag = 1.0;

        return f;
    }

}