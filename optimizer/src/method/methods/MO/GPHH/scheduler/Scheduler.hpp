#pragma once
#include <string>
#include <vector>
#include <array>
#include "../domain/Instance.hpp"
#include "../rules/IDispatchingRule.hpp"
#include "../rules/GPTreeRule.hpp"
#include "../rules/GPTreeResRule.hpp"

struct ScheduleResult {
    int makespan = 0;
    double totalCost = 0.0;
    std::vector<int> assignedResByImopseTaskIndex;
};

struct FeatureStat {
    long long groups = 0;
    long long sumCandidates = 0;
    long long groupsAllEqual = 0;
    long long groupsAllZero = 0;
    long long sumUnique = 0;
    double    sumRange = 0.0;

    long long groupsWithNonFinite = 0;
    long long nonFiniteValues = 0;
};

static constexpr size_t RES_FEAT_COUNT = 9;
static constexpr size_t TASK_FEAT_COUNT = 23;

struct ResFeatureDiag {
    std::array<FeatureStat, RES_FEAT_COUNT> st{};
};

struct TaskFeatureDiag {
    std::array<FeatureStat, TASK_FEAT_COUNT> st{};
};


inline constexpr std::array<const char*, RES_FEAT_COUNT> RES_FEAT_NAMES = {
    "RES_WAGE",
    "RES_SKILL_LEVEL",
    "RES_FREE_TIME",
    "RES_MULTI_SKILL",
    "RES_UTILIZATION",
    "RES_WAGE_PER_LEVEL",
    "RES_SURPLUS_LEVEL",
    "RES_RELATIVE_WAGE",
    "RES_FUTURE_DEMAND"
};

inline constexpr std::array<const char*, TASK_FEAT_COUNT> TASK_FEAT_NAMES = {
    "DURATION",
    "REQ_LEVEL",
    "AVAIL_SKILL",
    "EST_PREC",
    "SUCC_COUNT",
    "CRITLEN",
    "SLACK",
    "AVAIL_GAP",
    "WAIT_RES",
    "TOT_PRED",
    "CHEAPEST_COST_NOW",
    "COST_PER_SKILL_NOW",
    "MIN_WAGE_AVAIL",
    "AVG_WAGE_AVAIL",
    "TEAM_SIZE_MIN_NOW",
    "NUM_TASKS",
    "NUM_RESOURCES",
    "NUM_SKILLS",
    "TASK_RES_COUNT",
    "AVG_RES_COST",
    "UNSCHED_TASKS",
    "MIN_FEASIBLE_COST_NOW",
    "COST_REGRET_NOW"
};

struct ResChoiceDiag {
    long long calls = 0;
    long long emptyCalls = 0;

    long long sumCandidates = 0;
    long long sumUniqueScores = 0;
    long long sumTiesMin = 0;
};

class Scheduler {
public:
    static ScheduleResult precedenceOnly(Instance& I, const IDispatchingRule& rule);
    static ScheduleResult withResources(Instance& I,
        const IDispatchingRule& ruleT,
        const GPTreeResRule* ruleR = nullptr);

    static void setPriorityKeys(const std::vector<float>* keys);
    static void setForcedResources(const std::vector<int>* forcedByTaskIndex);
    static void setResChoiceDiag(ResChoiceDiag* diag);
    static void setResFeatureDiag(ResFeatureDiag* diag);
    static void setTaskFeatureDiag(TaskFeatureDiag* diag);
};