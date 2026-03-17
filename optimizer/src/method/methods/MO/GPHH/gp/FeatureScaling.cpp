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

        double maxTaskResCount = 1.0;
        for (const auto& t : I.tasks) {
            int req = std::max(0, t.reqLevel);
            int cnt = 0;

            if (req <= 0) {
                cnt = (int)I.resources.size();
            }
            else {
                for (const auto& r : I.resources) {
                    auto it = r.skills.find(t.reqSkill);
                    int lvl = (it != r.skills.end()) ? it->second : 0;
                    if (lvl >= req) ++cnt;
                }
            }

            maxTaskResCount = std::max(maxTaskResCount, (double)cnt);
        }
        s.maxTaskResCount = std::max(1.0, maxTaskResCount);

        s.maxUnschedTasks = s.maxNumTasks;

        s.maxAvgResCostForSkill = std::max(1.0, maxSalary);
        s.maxMinWageAvail = std::max(1.0, maxSalary);
        s.maxAvgWageAvail = s.maxMinWageAvail;

        s.maxCheapestCostNow = std::max(1.0, maxSalary);
        s.maxCostPerSkillNow = s.maxCheapestCostNow;
        s.maxTeamSizeMinNow = std::max(1.0, (double)nRes);

        double maxMinFeasibleCostNow = 1.0;
        double maxCostRegretNow = 1.0;
        double maxResWagePerLevel = 1.0;

        for (const auto& t : I.tasks) {
            int req = std::max(0, t.reqLevel);

            std::vector<double> wages;
            wages.reserve(I.resources.size());

            for (const auto& r : I.resources) {
                int lvl = 0;
                auto it = r.skills.find(t.reqSkill);
                if (it != r.skills.end()) lvl = it->second;

                if (req <= 0 || lvl >= req) {
                    wages.push_back(r.salary);

                    double wagePerLevel = r.salary / (double)std::max(1, lvl);
                    maxResWagePerLevel = std::max(maxResWagePerLevel, wagePerLevel);
                }
            }

            if (!wages.empty()) {
                double cheapest = std::numeric_limits<double>::infinity();
                double second = std::numeric_limits<double>::infinity();

                for (double w : wages) {
                    if (w < cheapest) {
                        second = cheapest;
                        cheapest = w;
                    }
                    else if (w < second) {
                        second = w;
                    }
                }

                if (!std::isfinite(second)) second = cheapest;

                maxMinFeasibleCostNow = std::max(maxMinFeasibleCostNow, cheapest * (double)t.duration);
                maxCostRegretNow = std::max(maxCostRegretNow, (second - cheapest) * (double)t.duration);
            }
        }

        double maxResSurplusLevel = 1.0;
        double maxResRelativeWage = 1.0;
        double maxResReservePressure = 1.0;

        for (const auto& t : I.tasks) {
            int req = std::max(0, t.reqLevel);

            double cheapest = std::numeric_limits<double>::infinity();

            for (const auto& r : I.resources) {
                int lvl = 0;
                auto it = r.skills.find(t.reqSkill);
                if (it != r.skills.end()) lvl = it->second;

                if (req <= 0 || lvl >= req) {
                    cheapest = std::min(cheapest, r.salary);
                    maxResSurplusLevel = std::max(maxResSurplusLevel, (double)std::max(0, lvl - req));
                }
            }

            if (std::isfinite(cheapest)) {
                for (const auto& r : I.resources) {
                    int lvl = 0;
                    auto it = r.skills.find(t.reqSkill);
                    if (it != r.skills.end()) lvl = it->second;

                    if (req <= 0 || lvl >= req) {
                        double rel = r.salary - cheapest;
                        maxResRelativeWage = std::max(maxResRelativeWage, rel);
                    }
                }
            }
        }

        for (const auto& r : I.resources) {
            double demand = 0.0;
            double reservePressure = 0.0;

            for (const auto& t : I.tasks) {
                int req = std::max(0, t.reqLevel);

                int lvl = 0;
                auto it = r.skills.find(t.reqSkill);
                if (it != r.skills.end()) lvl = it->second;

                if (req > 0 && lvl < req) continue;

                int feasibleCount = 0;
                double cheapest = std::numeric_limits<double>::infinity();
                double second = std::numeric_limits<double>::infinity();

                for (const auto& rr : I.resources) {
                    auto jt = rr.skills.find(t.reqSkill);
                    int ll = (jt != rr.skills.end()) ? jt->second : 0;
                    if (req > 0 && ll < req) continue;

                    ++feasibleCount;

                    if (rr.salary < cheapest) {
                        second = cheapest;
                        cheapest = rr.salary;
                    }
                    else if (rr.salary < second) {
                        second = rr.salary;
                    }
                }

                demand += 1.0 / (double)std::max(1, feasibleCount);

                if (!std::isfinite(second)) second = cheapest;
                const double priceGap = std::max(0.0, second - cheapest);

                reservePressure +=
                    ((double)t.duration * priceGap) / (double)std::max(1, feasibleCount);
            }

            maxResReservePressure = std::max(maxResReservePressure, reservePressure);
        }

        s.maxMinFeasibleCostNow = std::max(1.0, maxMinFeasibleCostNow);
        s.maxCostRegretNow = std::max(1.0, maxCostRegretNow);
        s.maxResWagePerLevel = std::max(1.0, maxResWagePerLevel);

        s.maxResSurplusLevel = std::max(1.0, maxResSurplusLevel);
        s.maxResRelativeWage = std::max(1.0, maxResRelativeWage);
        s.maxResReservePressure = std::max(1.0, maxResReservePressure);

        s.maxWaitRes = std::max(1.0, projHorizon);
        s.maxEstPrec = std::max(1.0, projHorizon);
        s.maxCritLen = s.maxEstPrec;
        s.maxSlackPos = s.maxEstPrec;
        s.maxCriticalPressure = s.maxCritLen;

        s.maxSuccCount = s.maxNumTasks;
        s.maxDescCount = s.maxNumTasks;
        s.maxTotPred = s.maxNumTasks;

        g_scaling = s;
    }

    const FeatureScaling& getFeatureScaling() {
        return g_scaling;
    }

} // namespace gp