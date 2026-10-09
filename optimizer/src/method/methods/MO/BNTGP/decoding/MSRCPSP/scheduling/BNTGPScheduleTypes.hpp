#pragma once

#include <vector>

namespace bntgp::decoding::msrcpsp::scheduling
{
    struct BNTGPScheduleResult final
    {
        int makespan{0};
        double totalCost{0.0};
        std::vector<int> assignedResByImopseTaskIndex{};
    };

    struct BNTGPScheduleOptions final
    {
        bool computeObjectiveStats{false};
        bool keepTaskAssignedResources{false};
        bool captureAssignedResByImopse{true};
    };
}
