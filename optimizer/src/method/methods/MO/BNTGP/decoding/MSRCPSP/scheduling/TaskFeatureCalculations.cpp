#include "FeatureCalculationDetails.hpp"
#include "FeatureCalculationHelpers.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace bntgp::decoding::msrcpsp::scheduling::detail
{
void setOptionalTaskFeatureUsage(FeatureEvaluationState& state,
    bool needTaskStructureFeatures,
    bool needTaskResCount,
    bool needTaskAvgResCost,
    bool needTaskUnschedTasks,
    bool needTaskAvailabilityFeatures,
    bool needTaskCostNowFeatures,
    bool needTaskReleasePressure,
    bool needTaskCriticalPressure)
{
    state.needTaskStructureFeatures = needTaskStructureFeatures;
    state.needTaskResourceCount = needTaskResCount;
    state.needTaskAverageResourceCost = needTaskAvgResCost;
    state.needUnscheduledTaskCount = needTaskUnschedTasks;
    state.needTaskAvailabilityFeatures = needTaskAvailabilityFeatures;
    state.needTaskCostNowFeatures = needTaskCostNowFeatures;
    state.needTaskReleasePressure = needTaskReleasePressure;
    state.needTaskCriticalPressure = needTaskCriticalPressure;
}

void setOptionalResourceFeatureUsage(FeatureEvaluationState& state,
    bool needFutureBranchFit,
    bool needBottleneckPreservation,
    bool needSpecialistMisuse)
{
    state.needResourceFutureBranchFit = needFutureBranchFit;
    state.needResourceBottleneckPreservation = needBottleneckPreservation;
    state.needResourceSpecialistMisuse = needSpecialistMisuse;
}
void buildTaskEvalStepPrecomputed(FeatureEvaluationState& state,
    const Instance& I,
    int now,
    const std::vector<int>& readyTaskIdx)
{
    const size_t taskCount = I.tasks.size();

    if (state.taskStepFeatures.size() != taskCount) {
        state.taskStepFeatures.resize(taskCount);
    }

    if (state.taskStepStamp.size() != taskCount) {
        state.taskStepStamp.assign(taskCount, 0);
    }

    nextStamp(state.taskStepCurrentStamp);
    state.taskEvaluationStepReady = true;

    PriorityContext ctx;
    ctx.inst = &I;
    ctx.now = now;

    for (int ix : readyTaskIdx) {
        if (ix < 0 || ix >= (int)I.tasks.size()) continue;
        state.taskStepFeatures[ix] = computeFeatures(state, ctx, ix);
        state.taskStepStamp[ix] = state.taskStepCurrentStamp;
    }
}
Features computeFeaturesFast(FeatureEvaluationState& state, const PriorityContext& ctx, int taskIx) {
    if (state.taskEvaluationStepReady &&
        taskIx >= 0 &&
        taskIx < (int)state.taskStepFeatures.size() &&
        taskIx < (int)state.taskStepStamp.size() &&
        state.taskStepStamp[taskIx] == state.taskStepCurrentStamp) {
        return state.taskStepFeatures[taskIx];
    }

    return computeFeatures(state, ctx, taskIx);
}

void buildPairEvalStepPrecomputed(FeatureEvaluationState& state,
    const Instance& I,
    int now,
    const std::vector<int>& readyTaskIdx
) {
    const size_t taskCount = I.tasks.size();
    const size_t resCount = I.resources.size();

    if (state.pairBaseTaskFeatures.size() != taskCount) {
        state.pairBaseTaskFeatures.resize(taskCount);
    }

    if (state.pairBaseTaskStamp.size() != taskCount) {
        state.pairBaseTaskStamp.assign(taskCount, 0);
    }

    if (state.pairBaseResourceFeatures.size() != resCount) {
        state.pairBaseResourceFeatures.resize(resCount);
    }

    for (int ri = 0; ri < (int)resCount; ++ri) {
        state.pairBaseResourceFeatures[ri] =
            computeResourceStepBaseRaw(state, I.resources[ri], now);
    }

    if (state.needResourceFutureBranchFit) {
        state.pairFutureBranchFitResourceCount = (int)resCount;

        const size_t pairCount = taskCount * resCount;

        if (state.pairFutureBranchFitCache.size() != pairCount) {
            state.pairFutureBranchFitCache.resize(pairCount);
        }
        if (state.pairFutureBranchFitStamp.size() != pairCount) {
            state.pairFutureBranchFitStamp.assign(pairCount, 0);
        }
    }
    else {
        state.pairFutureBranchFitResourceCount = 0;
    }

    nextStamp(state.pairCurrentStamp);
    state.pairEvaluationStepReady = true;

    PriorityContext ctx;
    ctx.inst = &I;
    ctx.now = now;

    for (int ix : readyTaskIdx) {
        if (ix < 0 || ix >= (int)I.tasks.size()) continue;

        const Task& t = I.tasks[ix];
        state.pairBaseTaskFeatures[ix] = computeFeaturesFast(state, ctx, ix);
        state.pairBaseTaskStamp[ix] = state.pairCurrentStamp;

        if (state.needResourceFutureBranchFit) {
            auto cacheForResIndex = [&](int ri) {
                if (ri < 0 || ri >= (int)I.resources.size()) return;

                const Resource& rr = I.resources[ri];
                const size_t flatIx =
                    (size_t)ix * (size_t)state.pairFutureBranchFitResourceCount + (size_t)ri;

                state.pairFutureBranchFitCache[flatIx] =
                    futureBranchFitRaw(state, I, ix, t, rr);

                state.pairFutureBranchFitStamp[flatIx] = state.pairCurrentStamp;
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
Features computeFeatures(FeatureEvaluationState& state, const PriorityContext& ctx, int taskIx) {
    Features f{};
    const Instance& I = *ctx.inst;
    const Task& t = I.tasks[taskIx];

    f.duration = t.duration;
    f.reqLevel = (double)t.totalRequiredLevel();
    const int req = totalReq(state, t);

    const auto& S = *state.scaling;

    const bool needStructure =
        state.needTaskStructureFeatures ||
        state.needTaskReleasePressure ||
        state.needTaskCriticalPressure;

    const bool needResCount =
        state.needTaskResourceCount ||
        state.needTaskReleasePressure ||
        state.needTaskCriticalPressure;

    const bool needAvailability =
        state.needTaskAvailabilityFeatures ||
        state.needTaskCostNowFeatures;

    if (needResCount) {
        f.taskResCount = taskResCountFeature(state, I, taskIx, t, req);
    }

    if (state.needTaskAverageResourceCost) {
        f.avgResCostForSkill = avgSalaryForSkill(state, I, taskIx, t, req);
    }

    if (state.needUnscheduledTaskCount) {
        f.unschedTasks = (double)countUnschedTasks(state, I);
    }

    if (needAvailability) {
        bool predsDone = false;
        if (state.remainingPredecessorCounts &&
            taskIx >= 0 &&
            taskIx < (int)state.remainingPredecessorCounts->size()) {
            predsDone = ((*state.remainingPredecessorCounts)[taskIx] == 0);
        }
        else {
            predsDone = predecessorsDoneNow(state, I, t, ctx.now);
        }

        double availCached = -1.0;

        if (req > 0 && canUseSingleSkillCache(state, t) && !t.reqSkill.empty()) {
            availCached = cachedAvailSkill(state, t.reqSkill);
        }

        f.availSkill = (availCached >= 0.0)
            ? availCached
            : bestFreeMatchedLevel(state, I, taskIx, t, ctx.now);

        f.availGap = f.availSkill - (double)req;
        f.feasibleNow = predsDone && (f.availSkill >= (double)req);
    }

    if (needStructure) {
        if (auto cpm = state.cpm) {
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

    if (state.needTaskReleasePressure) {
        f.taskReleasePressure = taskReleasePressureRaw(state,
            t,
            f.taskResCount,
            f.critLen,
            f.slack,
            f.descCount
        );
    }

    if (state.needTaskCostNowFeatures) {
        fillCostNowFeatures(state, f, I, t, req, ctx.now);
    }

    normalizeTaskFeatures(state, f, S);

    if (state.needTaskCriticalPressure) {
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
}
