#pragma once
#include <vector>
#include <optional>
#include <string>
#include <unordered_map>
#include "domain/Instance.hpp"

class ResourceAllocator {
public:
    static std::vector<int> availableResourceIds(const Instance& I, int now);
    static int availableSkillSum(const Instance& I, int now, const std::string& skill);
    static double subsetCost(const Instance& I, const std::vector<int>& subsetIds);

    static int waitUntilFeasible(const Instance& I, int now,
        const std::string& skill, int reqLevel);

    static std::optional<std::vector<int>> cheapestSubset(const Instance& I,
        const std::string& skill,
        int reqLevel,
        int now);

    static void setPrecomputed(
        const std::unordered_map<std::string, std::vector<int>>* resIdsBySkill,
        const std::unordered_map<int, int>* resIndexById,
        const std::unordered_map<std::string, std::vector<int>>* skillLevelsBySkillIndex);
};
