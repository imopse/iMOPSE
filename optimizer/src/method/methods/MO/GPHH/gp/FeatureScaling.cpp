#include "FeatureScaling.hpp"
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <string>

namespace gp {

    static FeatureScaling g_scaling;

    void initFeatureScaling(const Instance& I) {
        FeatureScaling s;

        const int nTasks = (int)I.tasks.size();
        const int nRes = (int)I.resources.size();

        s.maxNumTasks = std::max(1.0, (double)nTasks);
        s.maxNumResources = std::max(1.0, (double)nRes);

        double maxDur = 0.0;
        double maxReq = 0.0;
        double projHorizon = 0.0;
        for (const auto& t : I.tasks) {
            maxDur = std::max(maxDur, (double)t.duration);
            maxReq = std::max(maxReq, (double)t.reqLevel);
            projHorizon += t.duration;
        }
        s.maxDuration = std::max(1.0, maxDur);
        s.maxReqLevel = std::max(1.0, maxReq);

        std::unordered_set<std::string> allSkills;
        std::unordered_map<std::string, int> skillSum;

        double maxSalary = 0.0;
        double maxResSkillLevelFound = 0.0;

        for (const auto& r : I.resources) {
            maxSalary = std::max(maxSalary, r.salary);
            for (const auto& kv : r.skills) {
                allSkills.insert(kv.first);
                skillSum[kv.first] += kv.second;
                maxResSkillLevelFound = std::max(maxResSkillLevelFound, (double)kv.second);
            }
        }

        s.maxNumSkills = std::max(1.0, (double)allSkills.size());
        s.maxResSkillLevel = std::max(1.0, maxResSkillLevelFound);

        int maxSkillSum = 0;
        for (const auto& kv : skillSum) {
            maxSkillSum = std::max(maxSkillSum, kv.second);
        }
        s.maxAvailSkill = std::max(1.0, (double)maxSkillSum);
        s.maxAvailGapPos = s.maxAvailSkill;

        s.maxUnschedTasks = s.maxNumTasks;

        s.maxAvgResCostForSkill = std::max(1.0, maxSalary);
        s.maxMinWageAvail = std::max(1.0, maxSalary);
        s.maxAvgWageAvail = s.maxMinWageAvail;

        s.maxCheapestCostNow = std::max(1.0, maxSalary);
        s.maxCostPerSkillNow = s.maxCheapestCostNow;
        s.maxTeamSizeMinNow = std::max(1.0, (double)nRes);

        s.maxWaitRes = std::max(1.0, projHorizon);
        s.maxEstPrec = std::max(1.0, projHorizon);
        s.maxCritLen = s.maxEstPrec;
        s.maxSlackPos = s.maxEstPrec;

        s.maxSuccCount = s.maxNumTasks;
        s.maxTotPred = s.maxNumTasks;

        g_scaling = s;
    }

    const FeatureScaling& getFeatureScaling() {
        return g_scaling;
    }

} // namespace gp