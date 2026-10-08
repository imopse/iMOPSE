#include "BNTGPFeatureEvaluator.hpp"

#include "FeatureCalculationDetails.hpp"

namespace bntgp::decoding::msrcpsp::scheduling
{
    BNTGPFeatureEvaluator::BNTGPFeatureEvaluator(
        const BNTGPSchedulingModel& model,
        const FeatureScaling& scaling,
        const CPMPrecalc& cpm,
        const BNTGPFeatureRequirements& requirements) noexcept
    {
        state_.scaling = &scaling;
        state_.cpm = &cpm;
        state_.taskResourceCounts = &model.taskResourceCounts();
        state_.averageResourceCosts = &model.averageResourceCosts();
        state_.resourceIndexById = &model.resourceIndexById();
        state_.matchedLevelByTaskResource =
            &model.matchedLevelByTaskResource();
        state_.matchedLevelResourceCount =
            static_cast<int>(model.resourceCount());

        detail::setOptionalTaskFeatureUsage(
            state_,
            requirements.needsTaskStructureFeatures(),
            requirements.taskResourceCount,
            requirements.taskAverageResourceCost,
            requirements.unscheduledTaskCount,
            requirements.needsTaskAvailabilityFeatures(),
            requirements.needsTaskCostNowFeatures(),
            requirements.taskReleasePressure,
            requirements.taskCriticalPressure
        );

        detail::setOptionalResourceFeatureUsage(
            state_,
            requirements.resourceFutureBranchFit,
            requirements.resourceBottleneckPreservation,
            requirements.resourceSpecialistMisuse
        );
    }

    void BNTGPFeatureEvaluator::bindScheduleState(
        const int& unscheduledCount,
        const std::vector<int>& remainingPredecessorCounts,
        const std::vector<int>& latestPredecessorFinish,
        const std::unordered_map<std::string, SkillStepInfo>& skillStepCache) noexcept
    {
        state_.unscheduledCount = &unscheduledCount;
        state_.remainingPredecessorCounts = &remainingPredecessorCounts;
        state_.latestPredecessorFinish = &latestPredecessorFinish;
        state_.skillStepCache = &skillStepCache;
    }

    void BNTGPFeatureEvaluator::prepareDecisionStep(
        const domain::Instance& instance,
        const int now,
        const std::vector<int>& readyTaskIndices)
    {
        detail::buildTaskEvalStepPrecomputed(
            state_,
            instance,
            now,
            readyTaskIndices
        );

        detail::buildPairEvalStepPrecomputed(
            state_,
            instance,
            now,
            readyTaskIndices
        );
    }

    Features BNTGPFeatureEvaluator::computePairFeatures(
        const domain::Instance& instance,
        const int taskIndex,
        const domain::Task& task,
        const domain::Resource& resource,
        const int now,
        const double cheapestNow,
        const double cheapestCapableOverall,
        const double waitOfCheapestCapableOverall,
        const double reservePressureExcludingTask,
        const double criticalReserveExcludingTask,
        const double familyMismatchExcludingTask)
    {
        return detail::computePairFeaturesFast(
            state_,
            instance,
            taskIndex,
            task,
            resource,
            now,
            cheapestNow,
            cheapestCapableOverall,
            waitOfCheapestCapableOverall,
            reservePressureExcludingTask,
            criticalReserveExcludingTask,
            familyMismatchExcludingTask
        );
    }
}
