#pragma once

#include "GPTypes.hpp"

#include <array>
#include <cstddef>
#include <string_view>

namespace bntgp::gp
{
    inline constexpr std::array<FeatureId, FeatureCount>
        FeatureSamplingOrder{
            FeatureId::DURATION,
            FeatureId::REQ_LEVEL,
            FeatureId::AVAIL_SKILL,
            FeatureId::CRITLEN,
            FeatureId::SLACK,
            FeatureId::DESC_COUNT,
            FeatureId::TASK_CRITICAL_PRESSURE,
            FeatureId::TASK_RELEASE_PRESSURE,
            FeatureId::AVAIL_GAP,
            FeatureId::CHEAPEST_COST_NOW,
            FeatureId::COST_PER_SKILL_NOW,
            FeatureId::TASK_RES_COUNT,
            FeatureId::AVG_RES_COST,
            FeatureId::UNSCHED_TASKS,
            FeatureId::MIN_FEASIBLE_COST_NOW,
            FeatureId::COST_REGRET_NOW,

            FeatureId::RES_WAGE,
            FeatureId::RES_SKILL_LEVEL,
            FeatureId::RES_IDLE_TIME,
            FeatureId::RES_CAN_START_NOW,
            FeatureId::RES_UTILIZATION,
            FeatureId::RES_WAGE_PER_LEVEL,
            FeatureId::RES_ASSIGN_COST,
            FeatureId::RES_ASSIGN_PREMIUM_ALL,
            FeatureId::RES_RESERVE_PRESSURE,
            FeatureId::RES_FAMILY_MISMATCH,
            FeatureId::RES_FUTURE_BRANCH_FIT,
            FeatureId::RES_BOTTLENECK_PRESERVATION,
            FeatureId::RES_SPECIALIST_MISUSE,
            FeatureId::RES_RELATIVE_WAGE
    };

    [[nodiscard]]
    constexpr std::size_t toFeatureIndex(
        const FeatureId feature) noexcept
    {
        return static_cast<std::size_t>(feature);
    }

    [[nodiscard]]
    constexpr bool isValidFeature(
        const FeatureId feature) noexcept
    {
        return toFeatureIndex(feature) < FeatureCount;
    }

    [[nodiscard]]
    constexpr std::size_t samplingIndex(
        const FeatureId feature) noexcept
    {
        for (std::size_t index = 0U;
            index < FeatureSamplingOrder.size();
            ++index)
        {
            if (FeatureSamplingOrder[index] == feature)
            {
                return index;
            }
        }

        return FeatureCount;
    }

    [[nodiscard]]
    constexpr std::string_view featureShortName(
        const FeatureId feature) noexcept
    {
        switch (feature)
        {
        case FeatureId::DURATION:
            return "DUR";

        case FeatureId::REQ_LEVEL:
            return "REQ";

        case FeatureId::AVAIL_SKILL:
            return "AVAIL";

        case FeatureId::CRITLEN:
            return "CRITLEN";

        case FeatureId::SLACK:
            return "SLACK";

        case FeatureId::DESC_COUNT:
            return "DESC_COUNT";

        case FeatureId::TASK_RELEASE_PRESSURE:
            return "REL_PRESS";

        case FeatureId::TASK_CRITICAL_PRESSURE:
            return "TASK_CRIT";

        case FeatureId::AVAIL_GAP:
            return "GAP";

        case FeatureId::CHEAPEST_COST_NOW:
            return "CHEAP";

        case FeatureId::COST_PER_SKILL_NOW:
            return "CHEAP_PER_SK";

        case FeatureId::TASK_RES_COUNT:
            return "TASK_RES";

        case FeatureId::AVG_RES_COST:
            return "AVG_RES_COST";

        case FeatureId::UNSCHED_TASKS:
            return "UNSCHED";

        case FeatureId::MIN_FEASIBLE_COST_NOW:
            return "MIN_COST_NOW";

        case FeatureId::COST_REGRET_NOW:
            return "REGRET_NOW";

        case FeatureId::RES_WAGE:
            return "RES_WAGE";

        case FeatureId::RES_SKILL_LEVEL:
            return "RES_SKILL";

        case FeatureId::RES_IDLE_TIME:
            return "RES_IDLE";

        case FeatureId::RES_CAN_START_NOW:
            return "RES_CAN_NOW";

        case FeatureId::RES_UTILIZATION:
            return "RES_UTIL";

        case FeatureId::RES_WAGE_PER_LEVEL:
            return "RES_W_PER_L";

        case FeatureId::RES_ASSIGN_COST:
            return "RES_ASSIGN_COST";

        case FeatureId::RES_ASSIGN_PREMIUM_ALL:
            return "RES_PREMIUM";

        case FeatureId::RES_RESERVE_PRESSURE:
            return "RES_RESERVE";

        case FeatureId::RES_FAMILY_MISMATCH:
            return "RES_FAM_MIS";

        case FeatureId::RES_FUTURE_BRANCH_FIT:
            return "RES_FUT_BRANCH";

        case FeatureId::RES_BOTTLENECK_PRESERVATION:
            return "RES_BOTTLENECK";

        case FeatureId::RES_SPECIALIST_MISUSE:
            return "RES_SPEC_MIS";

        case FeatureId::RES_RELATIVE_WAGE:
            return "RES_REL_WAGE";

        case FeatureId::COUNT:
            break;
        }

        return "UNKNOWN";
    }

    namespace detail
    {
        [[nodiscard]]
        constexpr bool containsUniqueFeatures() noexcept
        {
            for (std::size_t first = 0U;
                first < FeatureSamplingOrder.size();
                ++first)
            {
                if (!isValidFeature(FeatureSamplingOrder[first]))
                {
                    return false;
                }

                for (std::size_t second = first + 1U;
                    second < FeatureSamplingOrder.size();
                    ++second)
                {
                    if (FeatureSamplingOrder[first] ==
                        FeatureSamplingOrder[second])
                    {
                        return false;
                    }
                }
            }

            return true;
        }

        [[nodiscard]]
        constexpr bool containsEveryFeature() noexcept
        {
            for (std::size_t featureIndex = 0U;
                featureIndex < FeatureCount;
                ++featureIndex)
            {
                const auto feature =
                    static_cast<FeatureId>(featureIndex);

                if (samplingIndex(feature) == FeatureCount)
                {
                    return false;
                }
            }

            return true;
        }
    }

    static_assert(
        FeatureSamplingOrder.size() == FeatureCount,
        "The BNTGP feature pool must contain exactly 30 features.");

    static_assert(
        detail::containsUniqueFeatures(),
        "The BNTGP feature pool contains an invalid or duplicated feature.");

    static_assert(
        detail::containsEveryFeature(),
        "The BNTGP feature pool does not contain every FeatureId.");

    static_assert(
        FeatureSamplingOrder[6] ==
        FeatureId::TASK_CRITICAL_PRESSURE);

    static_assert(
        FeatureSamplingOrder[7] ==
        FeatureId::TASK_RELEASE_PRESSURE);
}