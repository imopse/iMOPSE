#include "ResourceAllocator.hpp"
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <limits>
#include <optional>    
#include <cmath>


static const std::unordered_map<std::string, std::vector<int>>* g_resBySkill = nullptr;
static const std::unordered_map<int, int>* g_resIndexById = nullptr;
static const std::unordered_map<std::string, std::vector<int>>* g_skillLevelsMap = nullptr;


void ResourceAllocator::setPrecomputed(
    const std::unordered_map<std::string, std::vector<int>>* resIdsBySkill,
    const std::unordered_map<int, int>* resIndexById,
    const std::unordered_map<std::string, std::vector<int>>* skillLevelsBySkillIndex)
{
    g_resBySkill = resIdsBySkill;
    g_resIndexById = resIndexById;
    g_skillLevelsMap = skillLevelsBySkillIndex;
}


std::vector<int> ResourceAllocator::availableResourceIds(const Instance& I, int now) {
    std::vector<int> ids;
    ids.reserve(I.resources.size());
    for (const auto& r : I.resources)
        if (r.busyUntil <= now) ids.push_back(r.id);
    return ids;
}


static inline int skillLevelOfInst(const Instance& I, int resId, const std::string& skill) {
    auto it = std::find_if(I.resources.begin(), I.resources.end(),
        [&](const Resource& x) { return x.id == resId; });
    if (it == I.resources.end()) return 0;
    auto jt = it->skills.find(skill);
    return (jt == it->skills.end() ? 0 : jt->second);
}

static inline const Resource* findResById(const Instance& I, int id) {
    auto it = std::find_if(I.resources.begin(), I.resources.end(),
        [&](const Resource& x) { return x.id == id; });
    return (it == I.resources.end() ? nullptr : &*it);
}

double ResourceAllocator::subsetCost(const Instance& I, const std::vector<int>& subsetIds) {
    double s = 0.0;
    for (int id : subsetIds) {
        auto it = std::find_if(I.resources.begin(), I.resources.end(),
            [&](const Resource& r) { return r.id == id; });
        if (it != I.resources.end()) s += it->salary;
    }
    return s;
}

int ResourceAllocator::availableSkillSum(const Instance& I, int now,
    const std::string& skill)
{
    int best = 0;

    auto scanOne = [&](const Resource& r) {
        if (r.busyUntil > now) return;
        int lvl = 0;
        auto it = r.skills.find(skill);
        if (it != r.skills.end()) lvl = it->second;
        if (lvl > best) best = lvl;
        };

    if (g_resBySkill && g_resIndexById) {
        auto it = g_resBySkill->find(skill);
        if (it != g_resBySkill->end()) {
            for (int rid : it->second) {
                auto jt = g_resIndexById->find(rid);
                if (jt == g_resIndexById->end()) continue;
                scanOne(I.resources[jt->second]);
            }
            return best;
        }
    }

    for (const auto& r : I.resources) scanOne(r);
    return best;
}

std::optional<std::vector<int>> ResourceAllocator::cheapestSubset(const Instance& I,
    const std::string& skill, int reqLevel, int now)
{
    int bestId = -1;
    double bestSalary = std::numeric_limits<double>::infinity();

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

    if (bestId < 0) return std::nullopt;
    return std::vector<int>{ bestId };
}


int ResourceAllocator::waitUntilFeasible(const Instance& I, int now,
    const std::string& skill, int reqLevel)
{
    int bestWait = std::numeric_limits<int>::max();

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

