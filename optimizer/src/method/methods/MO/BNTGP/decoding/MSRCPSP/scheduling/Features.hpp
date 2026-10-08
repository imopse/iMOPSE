#pragma once
#include <string>
#include <vector>
#include <limits>
#include "../domain/Instance.hpp"

namespace bntgp::decoding::msrcpsp::scheduling
{
using domain::Instance;
using domain::Resource;
using domain::Task;

struct PriorityContext {
    const Instance* inst = nullptr;
    int now = 0;
};

struct Features {
    double duration = 0.0;
    double reqLevel = 0.0;
    double availSkill = 0.0;

    bool feasibleNow = false;

    double critLen = 0.0;
    double slack = 0.0;
    double descCount = 0.0;
    double taskReleasePressure = 0.0;
    double taskCriticalPressure = 0.0;
    double availGap = 0.0;

    double cheapestCostNow = std::numeric_limits<double>::infinity();
    double costPerSkillNow = std::numeric_limits<double>::infinity();

    double minFeasibleCostNow = std::numeric_limits<double>::infinity();
    double costRegretNow = 0.0;

    double resWage = 0.0;
    double resSkillLevel = 0.0;
    double resIdleTime = 0.0;
    double resCanStartNow = 0.0;
    double resUtilization = 0.0;
    double resWagePerLevel = 0.0;
    double resAssignCost = 0.0;
    double resAssignPremiumAll = 0.0;
    double resReservePressure = 0.0;
    double resFamilyMismatch = 0.0;
    double resFutureBranchFit = 0.0;
    double resBottleneckPreservation = 0.0;
    double resSpecialistMisuse = 0.0;
    double resRelativeWage = 0.0;

    double taskResCount = 0.0;
    double avgResCostForSkill = 0.0;
    double unschedTasks = 0.0;
};

struct SkillStepInfo {
    int maxFreeLevel = 0;
    std::vector<int> minWaitAtLeast;
    std::vector<double> cheapestAtLeast;
    std::vector<double> secondCheapestAtLeast;
};

}
