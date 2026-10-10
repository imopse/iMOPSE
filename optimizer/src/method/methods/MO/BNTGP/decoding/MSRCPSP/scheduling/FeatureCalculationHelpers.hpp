#pragma once

#include "FeatureEvaluationState.hpp"

namespace bntgp::decoding::msrcpsp::scheduling::detail
{
    double inf();

    double clamp01(double x);

    double normalize(double val, double maxVal);

    std::uint64_t nextStamp(std::uint64_t& current);

    bool predecessorsDoneNow(FeatureEvaluationState& state, const Instance& I, const Task& t, int now);

    int countUnschedTasks(FeatureEvaluationState& state, const Instance& I);

    bool canUseSingleSkillCache(FeatureEvaluationState& state, const Task& t);

    int totalReq(FeatureEvaluationState& state, const Task& t);

    int matchedLevelCached(FeatureEvaluationState& state,
            const Instance& I,
            int taskIx,
            const Task& t,
            const Resource& r);

    int resourceIndexCached(FeatureEvaluationState& state, const Resource& r);

    ResourceStepBase computeResourceStepBaseRaw(FeatureEvaluationState& state,
            const Resource& r,
            int now);

    bool tryGetResourceStepBaseCached(FeatureEvaluationState& state,
            const Resource& r,
            ResourceStepBase& outBase);

    void assignResourceStepBaseToFeatures(FeatureEvaluationState& state,
            const ResourceStepBase& base,
            Features& f);

    bool tryGetFutureBranchFitStepCached(FeatureEvaluationState& state,
            int taskIx,
            const Resource& r,
            double& outValue
        );

    double bestFreeMatchedLevel(FeatureEvaluationState& state, const Instance& I, int taskIx, const Task& t, int now);

    double taskResCountFeature(FeatureEvaluationState& state, const Instance& I, int taskIx, const Task& t, int req);

    double avgSalaryForSkill(FeatureEvaluationState& state, const Instance& I, int taskIx, const Task& t, int req);

    const SkillStepInfo* skillStepInfoFor(FeatureEvaluationState& state, const std::string& skill);

    double cachedAvailSkill(FeatureEvaluationState& state, const std::string& skill);

    double cachedWaitRes(FeatureEvaluationState& state, const std::string& skill, int req);

    bool cachedCheapestPair(FeatureEvaluationState& state, const std::string& skill, int req, double& first, double& second);

    void fillCostNowFeatures(FeatureEvaluationState& state, Features& f, const Instance& I, const Task& t, int req, int now);

    double bottleneckPreservationRaw(FeatureEvaluationState& state,
            const Instance& I,
            int taskIx,
            const Task& t,
            double criticalReserveExcludingTask);

    double specialistMisuseRaw(FeatureEvaluationState& state,
            const Instance& I,
            int taskIx,
            const Task& t,
            const Resource& r,
            double criticalReserveExcludingTask);

    double taskReleasePressureRaw(FeatureEvaluationState& state,
            const Task& t,
            double taskResCountRaw,
            double critLenRaw,
            double slackRaw,
            double descCountRaw);

    double fitGapRatioRaw(FeatureEvaluationState& state,
            const Instance& I,
            int taskIx,
            const Task& t,
            const Resource& r);

    void normalizeTaskFeatures(FeatureEvaluationState& state, Features& f, const FeatureScaling& S);

    void ensureDescendantCache(FeatureEvaluationState& state, const Instance& I);

    void ensureNearDescendantCache(FeatureEvaluationState& state, const Instance& I);

    double futureBranchFitRaw(FeatureEvaluationState& state,
            const Instance& I,
            int taskIx,
            const Task& t,
            const Resource& r);

    double futureBranchFitCached(FeatureEvaluationState& state,
            const Instance& I,
            int taskIx,
            const Task& t,
            const Resource& r
        );

}
