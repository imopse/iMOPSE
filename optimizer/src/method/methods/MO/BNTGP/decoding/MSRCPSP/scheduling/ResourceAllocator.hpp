#pragma once

#include "../domain/Instance.hpp"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace bntgp::decoding::msrcpsp::scheduling
{
    using domain::Instance;
    using domain::Resource;

    class ResourceAllocator final
    {
    public:
        ResourceAllocator(
            const std::unordered_map<std::string, std::vector<int>>& resourceIdsBySkill,
            const std::unordered_map<int, int>& resourceIndexById,
            const std::unordered_map<std::string, std::vector<int>>& skillLevelsBySkill
        ) noexcept;

        [[nodiscard]]
        int availableSkillSum(
            const Instance& instance,
            int now,
            const std::string& skill
        ) const;

        [[nodiscard]]
        int waitUntilFeasible(
            const Instance& instance,
            int now,
            const std::string& skill,
            int requiredLevel
        ) const;

        [[nodiscard]]
        int cheapestSubsetSingleId(
            const Instance& instance,
            const std::string& skill,
            int requiredLevel,
            int now
        ) const;

        [[nodiscard]]
        std::optional<std::vector<int>> cheapestSubset(
            const Instance& instance,
            const std::string& skill,
            int requiredLevel,
            int now
        ) const;

    private:
        const std::unordered_map<std::string, std::vector<int>>& resourceIdsBySkill_;
        const std::unordered_map<int, int>& resourceIndexById_;
        const std::unordered_map<std::string, std::vector<int>>& skillLevelsBySkill_;
    };
}
