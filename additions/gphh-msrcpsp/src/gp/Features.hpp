#pragma once
#include <string>
#include <vector>
#include <optional>
#include <limits>
#include "domain/Instance.hpp"
#include "alloc/ResourceAllocator.hpp"

struct PriorityContext {
    const Instance* inst = nullptr;
    int now = 0;
};

struct Features {
    double duration = 0.0;
    double reqLevel = 0.0;
    double availSkill = 0.0;
    double estPrec = 0.0;
    double succCount = 0.0;

    bool feasibleNow = false;

    double critLen = 0.0;
    double slack   = 0.0;
    double availGap= 0.0;
    double waitRes = 0.0;
    double totPred = 0.0;

    double cheapestCostNow = std::numeric_limits<double>::infinity();
    double costPerSkillNow = std::numeric_limits<double>::infinity();
    double minWageAvail = std::numeric_limits<double>::infinity();
    double avgWageAvail = std::numeric_limits<double>::infinity();
    double teamSizeMinNow = std::numeric_limits<double>::infinity();

    double RES_UTIL = 0.0;
    double RES_WAGE = 0.0;
    double RES_SKILL = 0.0;
    double RES_FREE = 0.0;
    double RES_MULTI = 0.0;

    double resWage = 0.0;
    double resSkillLevel = 0.0;
    double resFreeTime = 0.0;
    double resMultiSkill = 0.0;
    double resUtilization = 0.0;

    double numTasks = 0.0;
    double numResources = 0.0;
    double numSkills = 0.0;
    double taskResCount = 0.0;

    double avgResCostForSkill = 0.0;


    double unschedTasks = 0.0;
};

Features computeFeatures(const PriorityContext& ctx, int taskIx);
Features computeResourceFeatures(const Instance& I, const Task& t, const Resource& r, int now);

namespace gp {

    struct FeatureScaling {
        double maxDuration = 1.0;
        double maxReqLevel = 1.0;
        double maxNumTasks = 1.0;
        double maxResSkillLevel = 1.0;
        double maxNumResources = 1.0;
        double maxNumSkills = 1.0;
        double maxTaskResCount = 1.0;
        double maxAvgResCostForSkill = 1.0;
        double maxUnschedTasks = 1.0;
        double maxAvailSkill = 1.0;
        double maxAvailGapPos = 1.0;
        double maxWaitRes = 1.0;
        double maxEstPrec = 1.0;
        double maxCritLen = 1.0;
        double maxSlackPos = 1.0;
        double maxSuccCount = 1.0;
        double maxTotPred = 1.0;
        double maxCheapestCostNow = 1.0;
        double maxCostPerSkillNow = 1.0;
        double maxTeamSizeMinNow = 1.0;
        double maxMinWageAvail = 1.0;
        double maxAvgWageAvail = 1.0;
    };

    void initFeatureScaling(const Instance& I);

    const FeatureScaling& getFeatureScaling();

} // namespace gp
