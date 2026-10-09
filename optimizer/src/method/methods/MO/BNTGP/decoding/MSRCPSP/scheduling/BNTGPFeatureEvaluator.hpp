#pragma once

#include "BNTGPFeatureRequirements.hpp"
#include "BNTGPSchedulingModel.hpp"
#include "FeatureEvaluationState.hpp"
#include "Features.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace bntgp::decoding::msrcpsp::scheduling
{
    class BNTGPFeatureEvaluator final
    {
    public:
        BNTGPFeatureEvaluator(
            const BNTGPSchedulingModel& model,
            const FeatureScaling& scaling,
            const CPMPrecalc& cpm,
            const BNTGPFeatureRequirements& requirements
        ) noexcept;

        void bindScheduleState(
            const int& unscheduledCount,
            const std::vector<int>& remainingPredecessorCounts,
            const std::vector<int>& latestPredecessorFinish,
            const std::unordered_map<std::string, SkillStepInfo>& skillStepCache
        ) noexcept;

        void prepareDecisionStep(
            const domain::Instance& instance,
            int now,
            const std::vector<int>& readyTaskIndices
        );

        [[nodiscard]]
        Features computePairFeatures(
            const domain::Instance& instance,
            int taskIndex,
            const domain::Task& task,
            const domain::Resource& resource,
            int now,
            double cheapestNow,
            double cheapestCapableOverall,
            double waitOfCheapestCapableOverall,
            double reservePressureExcludingTask,
            double criticalReserveExcludingTask,
            double familyMismatchExcludingTask
        );

    private:
        detail::FeatureEvaluationState state_{};
    };
}
