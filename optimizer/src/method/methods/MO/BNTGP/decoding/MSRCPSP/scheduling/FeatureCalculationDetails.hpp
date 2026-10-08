#pragma once

#include "FeatureEvaluationState.hpp"

namespace bntgp::decoding::msrcpsp::scheduling::detail
{
    void setOptionalTaskFeatureUsage(
        FeatureEvaluationState& state,
        bool needTaskStructureFeatures,
        bool needTaskResourceCount,
        bool needTaskAverageResourceCost,
        bool needUnscheduledTaskCount,
        bool needTaskAvailabilityFeatures,
        bool needTaskCostNowFeatures,
        bool needTaskReleasePressure,
        bool needTaskCriticalPressure
    );

    void setOptionalResourceFeatureUsage(
        FeatureEvaluationState& state,
        bool needFutureBranchFit,
        bool needBottleneckPreservation,
        bool needSpecialistMisuse
    );
    void buildTaskEvalStepPrecomputed(
        FeatureEvaluationState& state,
        const Instance& instance,
        int now,
        const std::vector<int>& readyTaskIndices
    );
    void buildPairEvalStepPrecomputed(
        FeatureEvaluationState& state,
        const Instance& instance,
        int now,
        const std::vector<int>& readyTaskIndices
    );

    [[nodiscard]]
    Features computeFeatures(
        FeatureEvaluationState& state,
        const PriorityContext& context,
        int taskIndex
    );

    [[nodiscard]]
    Features computeFeaturesFast(
        FeatureEvaluationState& state,
        const PriorityContext& context,
        int taskIndex
    );
    [[nodiscard]]
    Features computeResourceFeaturesFast(
        FeatureEvaluationState& state,
        const Instance& instance,
        int taskIndex,
        const Task& task,
        const Resource& resource,
        int now,
        double cheapestNow,
        double cheapestCapableOverall,
        double waitOfCheapestCapableOverall,
        double reservePressureExcludingTask,
        double criticalReserveExcludingTask,
        double familyMismatchExcludingTask
    );

    [[nodiscard]]
    Features computePairFeaturesFast(
        FeatureEvaluationState& state,
        const Instance& instance,
        int taskIndex,
        const Task& task,
        const Resource& resource,
        int now,
        double cheapestNow,
        double cheapestCapableOverall,
        double waitOfCheapestCapableOverall,
        double reservePressureExcludingTask,
        double criticalReserveExcludingTask,
        double familyMismatchExcludingTask
    );
}
