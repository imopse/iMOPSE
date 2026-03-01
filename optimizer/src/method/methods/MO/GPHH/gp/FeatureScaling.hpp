#pragma once
#include "../domain/Instance.hpp"

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