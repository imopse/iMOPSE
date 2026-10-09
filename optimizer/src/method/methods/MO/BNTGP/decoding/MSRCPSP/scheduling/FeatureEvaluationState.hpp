#pragma once

#include "FeatureScaling.hpp"
#include "Features.hpp"
#include "Precompute.hpp"

#include <cstdint>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace bntgp::decoding::msrcpsp::scheduling::detail
{
    struct ResourceStepBase final
    {
        double resWage{0.0};
        double resIdleTime{0.0};
        double resCanStartNow{0.0};
        double resUtilization{0.0};
    };

    struct FeatureEvaluationState final
    {
        const FeatureScaling* scaling{nullptr};
        const CPMPrecalc* cpm{nullptr};

        const std::vector<double>* taskResourceCounts{nullptr};
        const std::vector<double>* averageResourceCosts{nullptr};
        const std::unordered_map<int, int>* resourceIndexById{nullptr};
        const int* unscheduledCount{nullptr};
        const std::vector<int>* remainingPredecessorCounts{nullptr};
        const std::vector<int>* latestPredecessorFinish{nullptr};
        const std::unordered_map<std::string, SkillStepInfo>* skillStepCache{nullptr};
        const std::vector<int>* matchedLevelByTaskResource{nullptr};
        int matchedLevelResourceCount{0};

        std::vector<Features> taskStepFeatures{};
        std::vector<std::uint64_t> taskStepStamp{};
        std::uint64_t taskStepCurrentStamp{1U};
        bool taskEvaluationStepReady{false};

        std::vector<Features> pairBaseTaskFeatures{};
        std::vector<std::uint64_t> pairBaseTaskStamp{};
        std::vector<ResourceStepBase> pairBaseResourceFeatures{};

        std::vector<double> pairFutureBranchFitCache{};
        std::vector<std::uint64_t> pairFutureBranchFitStamp{};
        std::uint64_t pairCurrentStamp{1U};
        int pairFutureBranchFitResourceCount{0};
        bool pairEvaluationStepReady{false};

        bool needResourceFutureBranchFit{true};
        bool needResourceBottleneckPreservation{true};
        bool needResourceSpecialistMisuse{true};
        bool needTaskStructureFeatures{true};
        bool needTaskResourceCount{true};
        bool needTaskAverageResourceCost{true};
        bool needUnscheduledTaskCount{true};
        bool needTaskAvailabilityFeatures{true};
        bool needTaskCostNowFeatures{true};
        bool needTaskReleasePressure{true};
        bool needTaskCriticalPressure{true};

        std::size_t descendantCacheTaskSignature{0U};
        int descendantCacheTaskCount{-1};
        std::vector<std::vector<int>> descendantsByTask{};

        std::size_t nearDescendantCacheTaskSignature{0U};
        int nearDescendantCacheTaskCount{-1};
        std::vector<std::vector<int>> nearDescendantsByTask{};
    };
}
