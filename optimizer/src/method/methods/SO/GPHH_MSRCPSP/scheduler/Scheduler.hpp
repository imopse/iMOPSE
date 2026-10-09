#pragma once
#include <vector>
#include <string>
#include <ostream>
#include "../domain/Instance.hpp"
#include "../rules/IDispatchingRule.hpp"
#include "../rules/GPTreeRule.hpp"
#include "../rules/GPTreeResRule.hpp"

namespace gphh_so {

struct ScheduleResult {
    int makespan = 0;
    double totalCost = 0.0;
    std::vector<int> assignedResByImopseTaskIndex;
};

struct ScheduleOptions {
    bool computeObjectiveStats = true;
    bool keepTaskAssignedResources = true;
    bool captureAssignedResByImopse = true;

    bool featureProfile = false;
    std::string featureProfileCsvPath;
};

class Scheduler {
public:
    static ScheduleResult withResources(Instance& I,
        const IDispatchingRule& ruleT,
        const GPTreeResRule* ruleR = nullptr,
        const ScheduleOptions& options = {},
        const GPTree* pairTree = nullptr);
};

}