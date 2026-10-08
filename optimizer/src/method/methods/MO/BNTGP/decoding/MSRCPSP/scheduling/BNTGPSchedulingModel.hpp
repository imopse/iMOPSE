#pragma once

#include "../domain/Instance.hpp"
#include "FeatureScaling.hpp"
#include "Features.hpp"
#include "Precompute.hpp"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace bntgp::decoding::msrcpsp::scheduling
{
    class BNTGPSchedulingModel final
    {
    public:
        BNTGPSchedulingModel(
            const domain::Instance& instance,
            const scheduling::FeatureScaling& scaling,
            const scheduling::CPMPrecalc& cpm
        );

        [[nodiscard]]
        std::size_t taskCount() const noexcept;

        [[nodiscard]]
        std::size_t resourceCount() const noexcept;

        [[nodiscard]]
        int familyCount() const noexcept;

        [[nodiscard]]
        int resourceIndex(int resourceId) const noexcept;

        void buildSkillStepCache(
            const domain::Instance& instance,
            int now,
            std::unordered_map<
                std::string,
                scheduling::SkillStepInfo
            >& output
        ) const;

        [[nodiscard]]
        const std::unordered_map<int, int>&
        resourceIndexById() const noexcept;

        [[nodiscard]]
        const std::unordered_map<std::string, std::vector<int>>&
        resourceIdsBySkill() const noexcept;

        [[nodiscard]]
        const std::unordered_map<std::string, std::vector<int>>&
        skillLevelsBySkill() const noexcept;

        [[nodiscard]]
        const std::vector<int>& baseIndegrees() const noexcept;

        [[nodiscard]]
        const std::vector<std::vector<int>>& successors() const noexcept;

        [[nodiscard]]
        const std::vector<double>& taskResourceCounts() const noexcept;

        [[nodiscard]]
        const std::vector<double>& averageResourceCosts() const noexcept;

        [[nodiscard]]
        const std::vector<std::vector<int>>& candidateResourceIndices(
        ) const noexcept;

        [[nodiscard]]
        const std::vector<int>& candidateResourceIndices(
            int taskIndex
        ) const noexcept;

        [[nodiscard]]
        const std::vector<int>& scoringResourceIndices(
            int taskIndex
        ) const noexcept;

        [[nodiscard]]
        const std::vector<int>& matchedLevelByTaskResource() const noexcept;

        [[nodiscard]]
        const std::vector<double>& reservePressureWeightByTask() const noexcept;

        [[nodiscard]]
        const std::vector<double>& criticalReserveWeightByTask() const noexcept;

        [[nodiscard]]
        const std::vector<int>& familyIdByTask() const noexcept;

        [[nodiscard]]
        const std::vector<double>& initialReservePressureByResource() const noexcept;

        [[nodiscard]]
        const std::vector<double>& initialCriticalReserveByResource() const noexcept;

        [[nodiscard]]
        const std::vector<double>& initialFamilyPressureByResourceFamily() const noexcept;

    private:
        std::size_t taskCount_{0U};
        std::size_t resourceCount_{0U};

        std::unordered_map<int, int> resourceIndexById_{};
        std::unordered_map<std::string, std::vector<int>> skillLevelsBySkill_{};
        std::unordered_map<std::string, std::vector<int>> resourceIdsBySkill_{};

        std::vector<int> baseIndegrees_{};
        std::vector<std::vector<int>> successors_{};

        std::vector<int> feasibleResourceCountByTask_{};
        std::vector<double> taskResourceCounts_{};
        std::vector<double> averageResourceCosts_{};
        std::vector<std::vector<int>> candidateResourceIndicesByTask_{};
        std::vector<std::vector<int>> scoringResourceIndicesByTask_{};
        std::vector<int> matchedLevelByTaskResource_{};

        std::vector<double> reservePressureWeightByTask_{};
        std::vector<double> criticalReserveWeightByTask_{};
        std::vector<int> familyIdByTask_{};
        int familyCount_{0};

        std::vector<double> initialReservePressureByResource_{};
        std::vector<double> initialCriticalReserveByResource_{};
        std::vector<double> initialFamilyPressureByResourceFamily_{};
    };
}
