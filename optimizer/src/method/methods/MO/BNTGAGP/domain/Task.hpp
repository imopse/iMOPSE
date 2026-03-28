#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include "Resource.hpp"

struct SkillRequirement {
    std::string skill;
    int level = 0;
};

struct Task {
    int id = 0;
    int duration = 0;

    std::string reqSkill;
    int reqLevel = 0;

    std::vector<SkillRequirement> requiredSkills;

    std::vector<int> predecessors;

    int imopseIndex = -1;

    std::vector<int> capableResources;
    std::vector<int> capableResourceIndices;

    int start = -1;
    int finish = -1;
    std::vector<int> assignedResources;

    int totalRequiredLevel() const {
        if (requiredSkills.empty()) {
            return std::max(0, reqLevel);
        }

        int sum = 0;
        for (const auto& rs : requiredSkills) {
            sum += std::max(0, rs.level);
        }
        return sum;
    }

    bool hasExplicitRequirements() const {
        return !requiredSkills.empty();
    }

    bool canBeDoneBy(const Resource& r) const {
        if (requiredSkills.empty()) {
            if (reqLevel <= 0) return true;
            auto it = r.skills.find(reqSkill);
            int lvl = (it != r.skills.end()) ? it->second : 0;
            return lvl >= reqLevel;
        }

        for (const auto& rs : requiredSkills) {
            auto it = r.skills.find(rs.skill);
            int lvl = (it != r.skills.end()) ? it->second : 0;
            if (lvl < rs.level) {
                return false;
            }
        }
        return true;
    }

    int matchedLevelOn(const Resource& r) const {
        if (requiredSkills.empty()) {
            auto it = r.skills.find(reqSkill);
            return (it != r.skills.end()) ? it->second : 0;
        }

        int sum = 0;
        for (const auto& rs : requiredSkills) {
            auto it = r.skills.find(rs.skill);
            if (it != r.skills.end()) {
                sum += it->second;
            }
        }
        return sum;
    }

    int surplusLevelOn(const Resource& r) const {
        if (requiredSkills.empty()) {
            auto it = r.skills.find(reqSkill);
            int lvl = (it != r.skills.end()) ? it->second : 0;
            return std::max(0, lvl - std::max(0, reqLevel));
        }

        int sum = 0;
        for (const auto& rs : requiredSkills) {
            auto it = r.skills.find(rs.skill);
            int lvl = (it != r.skills.end()) ? it->second : 0;
            sum += std::max(0, lvl - rs.level);
        }
        return sum;
    }

    std::string requirementKey() const {
        if (requiredSkills.empty()) {
            return reqSkill + "#" + std::to_string(std::max(0, reqLevel));
        }

        std::vector<std::pair<std::string, int>> norm;
        norm.reserve(requiredSkills.size());
        for (const auto& rs : requiredSkills) {
            norm.emplace_back(rs.skill, std::max(0, rs.level));
        }

        std::sort(norm.begin(), norm.end(),
            [](const auto& a, const auto& b) {
                if (a.first != b.first) return a.first < b.first;
                return a.second < b.second;
            });

        std::ostringstream oss;
        for (size_t i = 0; i < norm.size(); ++i) {
            if (i > 0) oss << "|";
            oss << norm[i].first << "#" << norm[i].second;
        }
        return oss.str();
    }
};