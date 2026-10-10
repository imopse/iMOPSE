#pragma once
#include "FeatureCatalog.hpp"
#include "GPTree.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>

namespace bntgp::gp {
    using GPFeatureMask = std::array<bool, FeatureCount>;
    inline constexpr std::array<std::string_view, FeatureCount> GPFeatureNames{
        "DURATION", "REQ_LEVEL", "AVAIL_SKILL", "CRITLEN", "SLACK", "DESC_COUNT",
        "TASK_RELEASE_PRESSURE", "TASK_CRITICAL_PRESSURE", "AVAIL_GAP",
        "CHEAPEST_COST_NOW", "COST_PER_SKILL_NOW", "TASK_RES_COUNT", "AVG_RES_COST",
        "UNSCHED_TASKS", "MIN_FEASIBLE_COST_NOW", "COST_REGRET_NOW", "RES_WAGE",
        "RES_SKILL_LEVEL", "RES_IDLE_TIME", "RES_CAN_START_NOW", "RES_UTILIZATION",
        "RES_WAGE_PER_LEVEL", "RES_ASSIGN_COST", "RES_ASSIGN_PREMIUM_ALL",
        "RES_RESERVE_PRESSURE", "RES_FAMILY_MISMATCH", "RES_FUTURE_BRANCH_FIT",
        "RES_BOTTLENECK_PRESERVATION", "RES_SPECIALIST_MISUSE", "RES_RELATIVE_WAGE"
    };
    [[nodiscard]] inline GPFeatureMask allGPFeatures() {
        GPFeatureMask mask{}; mask.fill(true); return mask;
    }
    [[nodiscard]] inline GPFeatureMask r23GPFeatures() {
        auto mask = allGPFeatures();
        for (const auto feature : {FeatureId::REQ_LEVEL, FeatureId::COST_PER_SKILL_NOW,
                 FeatureId::TASK_RES_COUNT, FeatureId::MIN_FEASIBLE_COST_NOW,
                 FeatureId::COST_REGRET_NOW, FeatureId::RES_RESERVE_PRESSURE,
                 FeatureId::RES_FUTURE_BRANCH_FIT}) {
            mask[toFeatureIndex(feature)] = false;
        }
        return mask;
    }
    [[nodiscard]] inline std::string normalizedFeatureName(std::string value) {
        std::string result;
        for (unsigned char c : value) {
            if (!std::isspace(c) && c != '-') result.push_back(static_cast<char>(std::toupper(c)));
        }
        return result;
    }
    [[nodiscard]] inline FeatureId featureFromName(const std::string& name) {
        const std::string canonical = normalizedFeatureName(name);
        for (std::size_t i = 0; i < FeatureCount; ++i) {
            auto id = static_cast<FeatureId>(i);
            if (canonical == GPFeatureNames[i] || canonical == featureShortName(id)) return id;
        }
        throw std::invalid_argument("BNTGP: unknown GP feature: " + name);
    }
    inline void applyFeatureList(GPFeatureMask& mask, const std::string& names, bool enabled) {
        std::size_t pos = 0;
        while (pos < names.size()) {
            std::size_t end = names.find(',', pos);
            if (end == std::string::npos) end = names.size();
            std::string name = names.substr(pos, end - pos);
            if (!normalizedFeatureName(name).empty()) mask[toFeatureIndex(featureFromName(name))] = enabled;
            pos = end + 1;
        }
        if (std::none_of(mask.begin(), mask.end(), [](bool active){return active;}))
            throw std::invalid_argument("BNTGP: at least one GP feature must be enabled");
    }
    struct ActiveGPFeatures {
        std::array<FeatureId, FeatureCount> ordered{};
        std::size_t count{0};
        ActiveGPFeatures() { set(r23GPFeatures()); }
        void set(const GPFeatureMask& mask) {
            count = 0;
            for (auto feature : FeatureSamplingOrder)
                if (mask[toFeatureIndex(feature)]) ordered[count++] = feature;
            if (count == 0) throw std::invalid_argument("BNTGP: feature mask is empty");
        }
    };
    inline thread_local ActiveGPFeatures CurrentGPFeatures{};
    inline void configureGPFeatures(const GPFeatureMask& mask) {CurrentGPFeatures.set(mask);}
    [[nodiscard]] inline FeatureId sampleActiveGPFeature(std::mt19937& randomEngine) {
        std::uniform_int_distribution<int> distribution(0, static_cast<int>(CurrentGPFeatures.count) - 1);
        return CurrentGPFeatures.ordered[static_cast<std::size_t>(distribution(randomEngine))];
    }
    [[nodiscard]] inline bool treeRespectsFeatureMask(const GPTree& tree, const GPFeatureMask& mask) {
        for (std::size_t i = 0; i < tree.nodeCount(); ++i) {
            const GPNode& n = tree.node(static_cast<NodeIndex>(i));
            if (n.kind == NodeKind::FEATURE && (!isValidFeature(n.feature) || !mask[toFeatureIndex(n.feature)]))
                return false;
        }
        return true;
    }
}
