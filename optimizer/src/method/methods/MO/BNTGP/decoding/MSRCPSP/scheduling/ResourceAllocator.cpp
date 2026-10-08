#include "ResourceAllocator.hpp"
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <limits>
#include <optional>    
#include <cmath>


namespace bntgp::decoding::msrcpsp::scheduling
{

ResourceAllocator::ResourceAllocator(
    const std::unordered_map<std::string, std::vector<int>>& resourceIdsBySkill,
    const std::unordered_map<int, int>& resourceIndexById,
    const std::unordered_map<std::string, std::vector<int>>& skillLevelsBySkill) noexcept
    : resourceIdsBySkill_(resourceIdsBySkill),
      resourceIndexById_(resourceIndexById),
      skillLevelsBySkill_(skillLevelsBySkill)
{
}

int ResourceAllocator::availableSkillSum(const Instance& I, int now,
    const std::string& skill) const
{
    int best = 0;

    auto scanOne = [&](const Resource& r) {
        if (r.busyUntil > now) return;
        int lvl = 0;
        auto it = r.skills.find(skill);
        if (it != r.skills.end()) lvl = it->second;
        if (lvl > best) best = lvl;
        };

    {
        auto it = resourceIdsBySkill_.find(skill);
        if (it != resourceIdsBySkill_.end()) {
            for (int rid : it->second) {
                auto jt = resourceIndexById_.find(rid);
                if (jt == resourceIndexById_.end()) continue;
                scanOne(I.resources[jt->second]);
            }
            return best;
        }
    }

    for (const auto& r : I.resources) scanOne(r);
    return best;
}

int ResourceAllocator::cheapestSubsetSingleId(const Instance& I,
    const std::string& skill, int reqLevel, int now) const
{
    int bestId = -1;
    double bestSalary = std::numeric_limits<double>::infinity();

    {
        auto itSkill = resourceIdsBySkill_.find(skill);
        if (itSkill != resourceIdsBySkill_.end()) {
            for (int rid : itSkill->second) {
                auto itIdx = resourceIndexById_.find(rid);
                if (itIdx == resourceIndexById_.end()) continue;

                const auto& r = I.resources[itIdx->second];
                if (r.busyUntil > now) continue;

                int lvl = 0;
                auto itLvl = skillLevelsBySkill_.find(skill);
                if (itLvl != skillLevelsBySkill_.end())
                {
                    lvl = itLvl->second[itIdx->second];
                }

                if (lvl < reqLevel) continue;

                if (r.salary < bestSalary) {
                    bestSalary = r.salary;
                    bestId = r.id;
                }
            }
            return bestId;
        }
    }

    for (const auto& r : I.resources) {
        if (r.busyUntil > now) continue;

        int lvl = 0;
        auto it = r.skills.find(skill);
        if (it != r.skills.end()) lvl = it->second;

        if (lvl < reqLevel) continue;

        if (r.salary < bestSalary) {
            bestSalary = r.salary;
            bestId = r.id;
        }
    }

    return bestId;
}

std::optional<std::vector<int>> ResourceAllocator::cheapestSubset(const Instance& I,
    const std::string& skill, int reqLevel, int now) const
{
    int bestId = cheapestSubsetSingleId(I, skill, reqLevel, now);
    if (bestId < 0) return std::nullopt;
    return std::vector<int>{ bestId };
}


int ResourceAllocator::waitUntilFeasible(const Instance& I, int now,
    const std::string& skill, int reqLevel) const
{
    int bestWait = std::numeric_limits<int>::max();

    {
        auto itSkill = resourceIdsBySkill_.find(skill);
        if (itSkill != resourceIdsBySkill_.end()) {
            for (int rid : itSkill->second) {
                auto itIdx = resourceIndexById_.find(rid);
                if (itIdx == resourceIndexById_.end()) continue;

                const auto& r = I.resources[itIdx->second];
                int lvl = 0;
                auto itLvl = skillLevelsBySkill_.find(skill);
                if (itLvl != skillLevelsBySkill_.end())
                {
                    lvl = itLvl->second[itIdx->second];
                }

                if (lvl < reqLevel) continue;

                if (r.busyUntil <= now) return 0;
                bestWait = std::min(bestWait, r.busyUntil - now);
            }

            return bestWait;
        }
    }

    for (const auto& r : I.resources) {
        int lvl = 0;
        auto it = r.skills.find(skill);
        if (it != r.skills.end()) lvl = it->second;

        if (lvl < reqLevel) continue;

        if (r.busyUntil <= now) return 0;
        bestWait = std::min(bestWait, r.busyUntil - now);
    }

    return bestWait;
}

}
