#pragma once

#include "FeatureCatalog.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <limits>
#include <type_traits>

namespace bntgp::gp
{
    inline constexpr std::size_t TaskFeatureCount = 16U;

    inline constexpr std::size_t ResourceFeatureCount =
        FeatureCount - TaskFeatureCount;

    [[nodiscard]]
    constexpr bool isTaskFeature(
        const FeatureId feature) noexcept
    {
        return isValidFeature(feature) &&
            toFeatureIndex(feature) < TaskFeatureCount;
    }

    [[nodiscard]]
    constexpr bool isResourceFeature(
        const FeatureId feature) noexcept
    {
        return isValidFeature(feature) &&
            toFeatureIndex(feature) >= TaskFeatureCount;
    }

    [[nodiscard]]
    constexpr std::size_t toResourceFeatureIndex(
        const FeatureId feature) noexcept
    {
        return toFeatureIndex(feature) - TaskFeatureCount;
    }

    static_assert(
        toFeatureIndex(FeatureId::COST_REGRET_NOW) + 1U ==
        TaskFeatureCount,
        "The last task feature must be COST_REGRET_NOW.");

    static_assert(
        toFeatureIndex(FeatureId::RES_WAGE) ==
        TaskFeatureCount,
        "The first resource feature must be RES_WAGE.");

    static_assert(
        ResourceFeatureCount == 14U,
        "BNTGP must contain exactly 14 resource-oriented features.");

    class TaskFeatureValues final
    {
    public:
        using Storage =
            std::array<double, TaskFeatureCount>;

        TaskFeatureValues() noexcept
        {
            reset();
        }

        void reset() noexcept
        {
            values_.fill(0.0);

            const double infinity =
                std::numeric_limits<double>::infinity();

            (*this)[FeatureId::CHEAPEST_COST_NOW] =
                infinity;

            (*this)[FeatureId::COST_PER_SKILL_NOW] =
                infinity;

            (*this)[FeatureId::MIN_FEASIBLE_COST_NOW] =
                infinity;
        }

        [[nodiscard]]
        double operator[](
            const FeatureId feature) const noexcept
        {
            assert(isTaskFeature(feature));

            return values_[toFeatureIndex(feature)];
        }

        double& operator[](
            const FeatureId feature) noexcept
        {
            assert(isTaskFeature(feature));

            return values_[toFeatureIndex(feature)];
        }

        [[nodiscard]]
        const Storage& raw() const noexcept
        {
            return values_;
        }

    private:
        Storage values_{};
    };

    class ResourceFeatureValues final
    {
    public:
        using Storage =
            std::array<double, ResourceFeatureCount>;

        void reset() noexcept
        {
            values_.fill(0.0);
        }

        [[nodiscard]]
        double operator[](
            const FeatureId feature) const noexcept
        {
            assert(isResourceFeature(feature));

            return values_[
                toResourceFeatureIndex(feature)
            ];
        }

        double& operator[](
            const FeatureId feature) noexcept
        {
            assert(isResourceFeature(feature));

            return values_[
                toResourceFeatureIndex(feature)
            ];
        }

        [[nodiscard]]
        const Storage& raw() const noexcept
        {
            return values_;
        }

    private:
        Storage values_{};
    };

    class FeatureValuesView final
    {
    public:
        FeatureValuesView(
            const TaskFeatureValues& taskValues,
            const ResourceFeatureValues& resourceValues
        ) noexcept
            : taskValues_(&taskValues),
            resourceValues_(&resourceValues)
        {
        }

        [[nodiscard]]
        double operator[](
            const FeatureId feature) const noexcept
        {
            assert(isValidFeature(feature));

            if (isTaskFeature(feature))
            {
                return (*taskValues_)[feature];
            }

            return (*resourceValues_)[feature];
        }

    private:
        const TaskFeatureValues* taskValues_;
        const ResourceFeatureValues* resourceValues_;
    };

    static_assert(
        std::is_trivially_copyable_v<TaskFeatureValues>,
        "TaskFeatureValues must remain trivially copyable.");

    static_assert(
        std::is_trivially_copyable_v<ResourceFeatureValues>,
        "ResourceFeatureValues must remain trivially copyable.");

    static_assert(
        std::is_trivially_copyable_v<FeatureValuesView>,
        "FeatureValuesView must remain trivially copyable.");

    static_assert(
        sizeof(TaskFeatureValues) ==
        TaskFeatureCount * sizeof(double),
        "TaskFeatureValues contains unexpected memory overhead.");

    static_assert(
        sizeof(ResourceFeatureValues) ==
        ResourceFeatureCount * sizeof(double),
        "ResourceFeatureValues contains unexpected memory overhead.");
}