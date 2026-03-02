#pragma once
#include <string>
#include <vector>
#include <optional>
#include <limits>
#include "../domain/Instance.hpp"

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

    double resWage = 0.0;
    double resSkill = 0.0;
    double resFree = 0.0;
    double resMulti = 0.0;
    double resUtil = 0.0;

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


