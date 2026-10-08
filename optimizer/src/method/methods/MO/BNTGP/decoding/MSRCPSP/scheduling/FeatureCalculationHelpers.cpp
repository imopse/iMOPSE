#include "FeatureCalculationHelpers.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace bntgp::decoding::msrcpsp::scheduling::detail
{
    double inf() { return std::numeric_limits<double>::infinity(); }

    double clamp01(double x) {
        if (!std::isfinite(x)) return 0.0;
        if (x < 0.0) return 0.0;
        if (x > 1.0) return 1.0;
        return x;
    }

    double normalize(double val, double maxVal) {
        if (!std::isfinite(val)) return 1.0;
        if (maxVal <= 0.0)       return 0.0;
        double x = val / maxVal;
        if (x < 0.0) x = 0.0;
        if (x > 1.0) x = 1.0;
        return x;
    }

    std::uint64_t nextStamp(std::uint64_t& current) {
        ++current;

        if (current == 0) {
            current = 1;
        }

        return current;
    }

    bool predecessorsDoneNow(FeatureEvaluationState& state, const Instance& I, const Task& t, int now) {
        for (int pid : t.predecessors) {
            auto it = I.idToIndex.find(pid);
            if (it != I.idToIndex.end()) {
                const Task& p = I.tasks[it->second];
                if (p.finish < 0 || p.finish > now) return false;
            }
        }
        return true;
    }
    int countUnschedTasks(FeatureEvaluationState& state, const Instance& I) {
        if (state.unscheduledCount) return *state.unscheduledCount;

        int unsched = 0;
        for (const auto& tt : I.tasks) if (tt.start < 0) ++unsched;
        return unsched;
    }

    bool canUseSingleSkillCache(FeatureEvaluationState& state, const Task& t) {
        return t.requiredSkills.empty() || t.requiredSkills.size() == 1;
    }

    int totalReq(FeatureEvaluationState& state, const Task& t) {
        return std::max(0, t.totalRequiredLevel());
    }
    int matchedLevelCached(FeatureEvaluationState& state, 
        const Instance& I,
        int taskIx,
        const Task& t,
        const Resource& r)
    {
        if (state.matchedLevelByTaskResource &&
            state.matchedLevelResourceCount > 0 &&
            state.resourceIndexById &&
            taskIx >= 0) {
            auto it = state.resourceIndexById->find(r.id);
            if (it != state.resourceIndexById->end()) {
                const int ri = it->second;
                const size_t idx =
                    (size_t)taskIx * (size_t)state.matchedLevelResourceCount + (size_t)ri;

                if (idx < state.matchedLevelByTaskResource->size()) {
                    return (*state.matchedLevelByTaskResource)[idx];
                }
            }
        }

        return t.matchedLevelOn(r);
    }

    int resourceIndexCached(FeatureEvaluationState& state, const Resource& r) {
        if (!state.resourceIndexById) return -1;

        auto it = state.resourceIndexById->find(r.id);
        if (it == state.resourceIndexById->end()) return -1;

        return it->second;
    }

    ResourceStepBase computeResourceStepBaseRaw(FeatureEvaluationState& state, 
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

    bool tryGetResourceStepBaseCached(FeatureEvaluationState& state, 
        const Resource& r,
        ResourceStepBase& outBase)
    {
        if (!state.pairEvaluationStepReady) return false;

        const int resIx = resourceIndexCached(state, r);
        if (resIx < 0) return false;
        if (resIx >= (int)state.pairBaseResourceFeatures.size()) return false;

        outBase = state.pairBaseResourceFeatures[resIx];
        return true;
    }

    void assignResourceStepBaseToFeatures(FeatureEvaluationState& state, 
        const ResourceStepBase& base,
        Features& f)
    {
        f.resWage = base.resWage;
        f.resIdleTime = base.resIdleTime;
        f.resCanStartNow = base.resCanStartNow;
        f.resUtilization = base.resUtilization;
    }

    bool tryGetFutureBranchFitStepCached(FeatureEvaluationState& state, 
        int taskIx,
        const Resource& r,
        double& outValue
    ) {
        if (!state.pairEvaluationStepReady) return false;
        if (taskIx < 0) return false;
        if (state.pairFutureBranchFitResourceCount <= 0) return false;

        const int resIx = resourceIndexCached(state, r);
        if (resIx < 0 || resIx >= state.pairFutureBranchFitResourceCount) return false;

        const size_t flatIx =
            (size_t)taskIx * (size_t)state.pairFutureBranchFitResourceCount + (size_t)resIx;

        if (flatIx >= state.pairFutureBranchFitStamp.size()) return false;
        if (state.pairFutureBranchFitStamp[flatIx] != state.pairCurrentStamp) return false;

        outValue = state.pairFutureBranchFitCache[flatIx];
        return true;
    }


    double bestFreeMatchedLevel(FeatureEvaluationState& state, const Instance& I, int taskIx, const Task& t, int now) {
        double best = 0.0;

        if (!t.capableResourceIndices.empty()) {
            for (int ri : t.capableResourceIndices) {
                if (ri < 0 || ri >= (int)I.resources.size()) continue;
                const auto& r = I.resources[ri];
                if (r.busyUntil > now) continue;
                best = std::max(best, (double)matchedLevelCached(state, I, taskIx, t, r));
            }
            return best;
        }

        for (const auto& r : I.resources) {
            if (r.busyUntil > now) continue;
            if (!t.canBeDoneBy(r)) continue;
            best = std::max(best, (double)matchedLevelCached(state, I, taskIx, t, r));
        }

        return best;
    }
    double taskResCountFeature(FeatureEvaluationState& state, const Instance& I, int taskIx, const Task& t, int req) {
        if (state.taskResourceCounts && taskIx >= 0 && taskIx < (int)state.taskResourceCounts->size()) {
            return (*state.taskResourceCounts)[taskIx];
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

    double avgSalaryForSkill(FeatureEvaluationState& state, const Instance& I, int taskIx, const Task& t, int req) {
        if (state.averageResourceCosts && taskIx >= 0 && taskIx < (int)state.averageResourceCosts->size()) {
            return (*state.averageResourceCosts)[taskIx];
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

    const SkillStepInfo* skillStepInfoFor(FeatureEvaluationState& state, const std::string& skill) {
        if (!state.skillStepCache) return nullptr;
        auto it = state.skillStepCache->find(skill);
        if (it == state.skillStepCache->end()) return nullptr;
        return &it->second;
    }

    double cachedAvailSkill(FeatureEvaluationState& state, const std::string& skill) {
        const SkillStepInfo* info = skillStepInfoFor(state, skill);
        if (!info) return -1.0;
        return (double)info->maxFreeLevel;
    }

    double cachedWaitRes(FeatureEvaluationState& state, const std::string& skill, int req) {
        const SkillStepInfo* info = skillStepInfoFor(state, skill);
        if (!info || req <= 0) return -1.0;
        if (req >= (int)info->minWaitAtLeast.size()) return (double)std::numeric_limits<int>::max();
        return (double)info->minWaitAtLeast[req];
    }

    bool cachedCheapestPair(FeatureEvaluationState& state, const std::string& skill, int req, double& first, double& second) {
        const SkillStepInfo* info = skillStepInfoFor(state, skill);
        if (!info || req <= 0) return false;
        if (req >= (int)info->cheapestAtLeast.size()) return false;

        first = info->cheapestAtLeast[req];
        second = info->secondCheapestAtLeast[req];

        return std::isfinite(first);
    }


    void fillCostNowFeatures(FeatureEvaluationState& state, Features& f, const Instance& I, const Task& t, int req, int now) {
        if (!f.feasibleNow) {
            f.cheapestCostNow = inf();
            f.costPerSkillNow = inf();
            f.minFeasibleCostNow = inf();
            f.costRegretNow = 0.0;
            return;
        }

        if (canUseSingleSkillCache(state, t) && t.capableResources.empty() && req > 0 && !t.reqSkill.empty()) {
            double first = std::numeric_limits<double>::infinity();
            double second = std::numeric_limits<double>::infinity();

            if (cachedCheapestPair(state, t.reqSkill, req, first, second)) {
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

    double bottleneckPreservationRaw(FeatureEvaluationState& state, 
        const Instance& I,
        int taskIx,
        const Task& t,
        double criticalReserveExcludingTask)
    {
        if (criticalReserveExcludingTask <= 0.0) return 0.0;

        const auto& S = *state.scaling;
        const int req = std::max(0, totalReq(state, t));
        const double feasibleCount = taskResCountFeature(state, I, taskIx, t, req);

        double critNorm = 0.0;
        double slackNorm = 0.0;

        if (auto cpm = state.cpm) {
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

    double specialistMisuseRaw(FeatureEvaluationState& state, 
        const Instance& I,
        int taskIx,
        const Task& t,
        const Resource& r,
        double criticalReserveExcludingTask)
    {
        if (criticalReserveExcludingTask <= 0.0) return 0.0;

        const auto& S = *state.scaling;
        const int req = std::max(0, totalReq(state, t));
        const double reqSafe = (double)std::max(1, req);
        const double feasibleCount = taskResCountFeature(state, I, taskIx, t, req);

        const double matched = (double)std::max(0, matchedLevelCached(state, I, taskIx, t, r));
        const double overkill =
            std::max(0.0, matched - reqSafe) / reqSafe;

        double critNorm = 0.0;
        double slackNorm = 0.0;

        if (auto cpm = state.cpm) {
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

    double taskReleasePressureRaw(FeatureEvaluationState& state, 
        const Task& t,
        double taskResCountRaw,
        double critLenRaw,
        double slackRaw,
        double descCountRaw)
    {
        const double reqRaw = (double)std::max(1, totalReq(state, t));
        const double scarcity = reqRaw / std::max(1.0, taskResCountRaw);
        const double urgency = (1.0 + std::max(0.0, critLenRaw))
            / (1.0 + std::max(0.0, slackRaw));
        const double branching = 1.0 + std::max(0.0, descCountRaw);

        return branching * urgency * scarcity;
    }

    double fitGapRatioRaw(FeatureEvaluationState& state, 
        const Instance& I,
        int taskIx,
        const Task& t,
        const Resource& r)
    {
        const int req = totalReq(state, t);
        if (req <= 0) return 0.0;

        const int lvl = std::max(0, matchedLevelCached(state, I, taskIx, t, r));
        if (lvl < req) return inf();

        return (double)std::max(0, lvl - req) / (double)std::max(1, req);
    }
    void normalizeTaskFeatures(FeatureEvaluationState& state, Features& f, const FeatureScaling& S) {
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
    void ensureDescendantCache(FeatureEvaluationState& state, const Instance& I) {
        const int N = (int)I.tasks.size();
        const std::size_t sig = I.taskStructureSignatureReady ? I.taskStructureSignature : 0;

        if ((int)state.descendantsByTask.size() == N) {
            if ((sig != 0 && state.descendantCacheTaskSignature == sig) ||
                (sig == 0 && state.descendantCacheTaskCount == N)) {
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

        state.descendantsByTask.assign(N, {});

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
                state.descendantsByTask[v].push_back(u);

                for (int s : succ[u]) {
                    stack.push_back(s);
                }
            }
        }

        state.descendantCacheTaskSignature = sig;
        state.descendantCacheTaskCount = N;
    }

    void ensureNearDescendantCache(FeatureEvaluationState& state, const Instance& I) {
        const int N = (int)I.tasks.size();
        const std::size_t sig = I.taskStructureSignatureReady ? I.taskStructureSignature : 0;

        if ((int)state.nearDescendantsByTask.size() == N) {
            if ((sig != 0 && state.nearDescendantCacheTaskSignature == sig) ||
                (sig == 0 && state.nearDescendantCacheTaskCount == N)) {
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

        state.nearDescendantsByTask.assign(N, {});

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
                state.nearDescendantsByTask[v].push_back(cur.node);

                if (cur.depth == MAX_DEPTH) continue;

                for (int s : succ[cur.node]) {
                    stack.push_back({ s, cur.depth + 1 });
                }
            }
        }

        state.nearDescendantCacheTaskSignature = sig;
        state.nearDescendantCacheTaskCount = N;
    }

    double futureBranchFitRaw(FeatureEvaluationState& state, 
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

        ensureNearDescendantCache(state, I);

        if (taskIx < 0 || taskIx >= (int)state.nearDescendantsByTask.size()) {
            return 0.0;
        }

        const auto& desc = state.nearDescendantsByTask[taskIx];
        if (desc.empty()) {
            return 0.0;
        }

        const auto& S = *state.scaling;

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

            if (auto cpm = state.cpm) {
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

            const int reqU = std::max(1, totalReq(state, u));
            const int lvlU = std::max(0, matchedLevelCached(state, I, uIx, u, r));
            const double overkill =
                std::max(0.0, (double)(lvlU - reqU) / (double)reqU);

            const double fitQuality = 1.0 / (1.0 + overkill);

            coveredWeight += weight * fitQuality;
        }

        if (totalWeight <= 1e-12) return 0.0;

        double branchFit = coveredWeight / totalWeight;

        const double currentGap = fitGapRatioRaw(state, I, taskIx, t, r);
        const double currentFit =
            std::isfinite(currentGap) ? (1.0 / (1.0 + currentGap)) : 0.0;

        const double finalValue =
            branchFit * (0.75 + 0.25 * currentFit);

        return clamp01(finalValue);
    }

    double futureBranchFitCached(FeatureEvaluationState& state, 
        const Instance& I,
        int taskIx,
        const Task& t,
        const Resource& r
    ) {
        if (!state.needResourceFutureBranchFit) {
            return 0.0;
        }

        double cachedValue = 0.0;
        if (tryGetFutureBranchFitStepCached(state, taskIx, r, cachedValue)) {
            return cachedValue;
        }

        return futureBranchFitRaw(state, I, taskIx, t, r);
    }
}
