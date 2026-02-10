#pragma once
#include <string>
#include <vector>
#include "../domain/Instance.hpp"
#include "../rules/IDispatchingRule.hpp"
#include "../rules/GPTreeRule.hpp"
#include "../rules/GPTreeResRule.hpp"

struct ScheduleResult {
    int makespan = 0;
    double totalCost = 0.0;
};

class Scheduler {
public:
    static ScheduleResult precedenceOnly(Instance& I, const IDispatchingRule& rule);
    static ScheduleResult withResources(Instance& I,
        const IDispatchingRule& ruleT,
        const GPTreeResRule* ruleR = nullptr);

    static void setPriorityKeys(const std::vector<float>* keys);
    static void setForcedResources(const std::vector<int>* forcedByTaskIndex);
};
