#include "FeatureScaling.hpp"
#include "Precompute.hpp"
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <cmath>
#include <limits>

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
            maxReq = std::max(maxReq, (double)t.totalRequiredLevel());
            projHorizon += t.duration;
        }
        s.maxDuration = std::max(1.0, maxDur);
        s.maxReqLevel = std::max(1.0, maxReq);

        std::unordered_set<std::string> allSkills;

        double maxSalary = 0.0;
        double maxResSkillLevelFound = 0.0;

        for (const auto& r : I.resources) {
            maxSalary = std::max(maxSalary, r.salary);
            for (const auto& kv : r.skills) {
                allSkills.insert(kv.first);
                maxResSkillLevelFound = std::max(maxResSkillLevelFound, (double)kv.second);
            }
        }

        s.maxNumSkills = std::max(1.0, (double)allSkills.size());
        s.maxResSkillLevel = std::max(1.0, maxResSkillLevelFound);

        double maxAvailSkill = 1.0;
        for (const auto& t : I.tasks) {
            for (const auto& r : I.resources) {
                if (!t.canBeDoneBy(r)) continue;
                maxAvailSkill = std::max(maxAvailSkill, (double)t.matchedLevelOn(r));
            }
        }
        s.maxAvailSkill = std::max(1.0, maxAvailSkill);
        s.maxAvailGapPos = s.maxAvailSkill;

        double maxTaskResCount = 1.0;
        for (const auto& t : I.tasks) {
            int req = std::max(0, t.totalRequiredLevel());
            int cnt = 0;

            if (req <= 0) {
                cnt = (int)I.resources.size();
            }
            else if (!t.capableResourceIndices.empty()) {
                cnt = (int)t.capableResourceIndices.size();
            }
            else {
                for (const auto& r : I.resources) {
                    if (t.canBeDoneBy(r)) ++cnt;
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
        double maxTaskReleasePressure = 1.0;

        for (const auto& t : I.tasks) {
            int req = std::max(0, t.totalRequiredLevel());

            std::vector<double> wages;
            wages.reserve(I.resources.size());

            for (const auto& r : I.resources) {
                if (req > 0 && !t.canBeDoneBy(r)) continue;

                wages.push_back(r.salary);

                double provided = (double)std::max(1, t.matchedLevelOn(r));
                double wagePerLevel = r.salary / provided;
                maxResWagePerLevel = std::max(maxResWagePerLevel, wagePerLevel);
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
        double maxResCriticalReserve = 1.0;
        double maxResBottleneckPreservation = 1.0;
        double maxResSpecialistMisuse = 1.0;

        std::vector<double> criticalReserveByResTmp(I.resources.size(), 0.0);

        CPMPrecalc cpm;
        const bool hasCPM = buildCPM(I, cpm);

        if (hasCPM) {
            for (int tIx = 0; tIx < nTasks; ++tIx) {
                const auto& t = I.tasks[tIx];
                const int req = std::max(0, t.totalRequiredLevel());

                int feasibleCount = 0;
                if (req <= 0) {
                    feasibleCount = (int)I.resources.size();
                }
                else if (!t.capableResourceIndices.empty()) {
                    feasibleCount = (int)t.capableResourceIndices.size();
                }
                else {
                    for (const auto& r : I.resources) {
                        if (t.canBeDoneBy(r)) ++feasibleCount;
                    }
                }

                const double taskResCountRaw = (double)std::max(1, feasibleCount);
                const double critLenRaw = (double)cpm.critLen[tIx];
                const double slackRaw = std::max(0.0, (double)cpm.slack[tIx]);
                const double descCountRaw = (double)cpm.descCount[tIx];

                const double reqRaw = (double)std::max(1, req);
                const double scarcity = reqRaw / std::max(1.0, taskResCountRaw);
                const double urgency = (1.0 + critLenRaw) / (1.0 + slackRaw);
                const double branching = 1.0 + descCountRaw;

                const double relPressure = branching * urgency * scarcity;
                maxTaskReleasePressure = std::max(maxTaskReleasePressure, relPressure);
            }
        }

        std::vector<std::vector<int>> succ(nTasks);

        for (int j = 0; j < nTasks; ++j) {
            for (int pid : I.tasks[j].predecessors) {
                auto it = I.idToIndex.find(pid);
                if (it != I.idToIndex.end()) {
                    succ[it->second].push_back(j);
                }
            }
        }

        for (const auto& t : I.tasks) {
            int req = std::max(0, t.totalRequiredLevel());

            double cheapest = std::numeric_limits<double>::infinity();

            for (const auto& r : I.resources) {
                if (req > 0 && !t.canBeDoneBy(r)) continue;

                cheapest = std::min(cheapest, r.salary);
                maxResSurplusLevel = std::max(maxResSurplusLevel, (double)t.surplusLevelOn(r));
            }

            if (std::isfinite(cheapest)) {
                for (const auto& r : I.resources) {
                    if (req > 0 && !t.canBeDoneBy(r)) continue;
                    double rel = r.salary - cheapest;
                    maxResRelativeWage = std::max(maxResRelativeWage, rel);
                }
            }
        }

        for (int ri = 0; ri < (int)I.resources.size(); ++ri) {
            const auto& r = I.resources[ri];
            double reservePressure = 0.0;
            double criticalReserve = 0.0;

            for (int tIx = 0; tIx < nTasks; ++tIx) {
                const auto& t = I.tasks[tIx];
                int req = std::max(0, t.totalRequiredLevel());

                if (req > 0 && !t.canBeDoneBy(r)) continue;

                int feasibleCount = 0;
                double cheapest = std::numeric_limits<double>::infinity();
                double second = std::numeric_limits<double>::infinity();

                for (const auto& rr : I.resources) {
                    if (req > 0 && !t.canBeDoneBy(rr)) continue;

                    ++feasibleCount;

                    if (rr.salary < cheapest) {
                        second = cheapest;
                        cheapest = rr.salary;
                    }
                    else if (rr.salary < second) {
                        second = rr.salary;
                    }
                }

                if (!std::isfinite(second)) second = cheapest;
                const double priceGap = std::max(0.0, second - cheapest);

                const double reserveContribution =
                    ((double)t.duration * priceGap) / (double)std::max(1, feasibleCount);

                reservePressure += reserveContribution;

                double critNorm = 0.0;
                double slackNorm = 0.0;
                double descNorm = 0.0;

                if (hasCPM) {
                    critNorm = std::min(
                        1.0,
                        (double)cpm.critLen[tIx] / std::max(1.0, projHorizon)
                    );
                    slackNorm = std::min(
                        1.0,
                        std::max(0.0, (double)cpm.slack[tIx]) / std::max(1.0, projHorizon)
                    );
                    descNorm = std::min(
                        1.0,
                        (double)cpm.descCount[tIx] / std::max(1.0, (double)nTasks)
                    );
                }

                const double structuralPressure =
                    (1.0 + critNorm + descNorm) / (1.0 + slackNorm);

                criticalReserve += reserveContribution * structuralPressure;
            }

            criticalReserveByResTmp[ri] = criticalReserve;

            maxResReservePressure = std::max(maxResReservePressure, reservePressure);
            maxResCriticalReserve = std::max(maxResCriticalReserve, criticalReserve);
        }
        for (int tIx = 0; tIx < nTasks; ++tIx) {
            const auto& t = I.tasks[tIx];
            const int req = std::max(0, t.totalRequiredLevel());

            int feasibleCount = 0;
            if (req <= 0) {
                feasibleCount = (int)I.resources.size();
            }
            else if (!t.capableResourceIndices.empty()) {
                feasibleCount = (int)t.capableResourceIndices.size();
            }
            else {
                for (const auto& rr : I.resources) {
                    if (t.canBeDoneBy(rr)) ++feasibleCount;
                }
            }

            double critNorm = 0.0;
            double slackNorm = 0.0;

            if (hasCPM) {
                critNorm = std::min(
                    1.0,
                    (double)cpm.critLen[tIx] / std::max(1.0, projHorizon)
                );
                slackNorm = std::min(
                    1.0,
                    std::max(0.0, (double)cpm.slack[tIx]) / std::max(1.0, projHorizon)
                );
            }

            const double reqSafe = (double)std::max(1, req);
            const double easyFactor =
                ((double)std::max(1, feasibleCount) / reqSafe)
                * (1.0 + slackNorm) / (1.0 + critNorm);

            if (req <= 0) {
                for (int ri = 0; ri < (int)I.resources.size(); ++ri) {
                    maxResBottleneckPreservation = std::max(
                        maxResBottleneckPreservation,
                        criticalReserveByResTmp[ri] * easyFactor
                    );
                }
            }
            else if (!t.capableResourceIndices.empty()) {
                for (int ri : t.capableResourceIndices) {
                    if (ri < 0 || ri >= (int)I.resources.size()) continue;

                    maxResBottleneckPreservation = std::max(
                        maxResBottleneckPreservation,
                        criticalReserveByResTmp[ri] * easyFactor
                    );
                }
            }
            else {
                for (int ri = 0; ri < (int)I.resources.size(); ++ri) {
                    const auto& rr = I.resources[ri];
                    if (!t.canBeDoneBy(rr)) continue;

                    maxResBottleneckPreservation = std::max(
                        maxResBottleneckPreservation,
                        criticalReserveByResTmp[ri] * easyFactor
                    );
                }
            }
        }
        for (int tIx = 0; tIx < nTasks; ++tIx) {
            const auto& t = I.tasks[tIx];
            const int req = std::max(0, t.totalRequiredLevel());

            int feasibleCount = 0;
            if (req <= 0) {
                feasibleCount = (int)I.resources.size();
            }
            else if (!t.capableResourceIndices.empty()) {
                feasibleCount = (int)t.capableResourceIndices.size();
            }
            else {
                for (const auto& rr : I.resources) {
                    if (t.canBeDoneBy(rr)) ++feasibleCount;
                }
            }

            double critNorm = 0.0;
            double slackNorm = 0.0;

            if (hasCPM) {
                critNorm = std::min(
                    1.0,
                    (double)cpm.critLen[tIx] / std::max(1.0, projHorizon)
                );
                slackNorm = std::min(
                    1.0,
                    std::max(0.0, (double)cpm.slack[tIx]) / std::max(1.0, projHorizon)
                );
            }

            const double reqSafe = (double)std::max(1, req);
            const double replaceability =
                (double)std::max(1, feasibleCount) / reqSafe;
            const double easyFactor =
                (1.0 + slackNorm) / (1.0 + critNorm);

            auto updateMisuseFor = [&](int ri) {
                if (ri < 0 || ri >= (int)I.resources.size()) return;

                const auto& rr = I.resources[ri];
                const double matched = (double)std::max(0, t.matchedLevelOn(rr));
                const double overkill =
                    std::max(0.0, matched - reqSafe) / reqSafe;

                const double misuse =
                    criticalReserveByResTmp[ri]
                    * replaceability
                        * easyFactor
                        * (1.0 + overkill);

                    maxResSpecialistMisuse = std::max(maxResSpecialistMisuse, misuse);
                };

            std::vector<double> futureCheapnessByResTmp(I.resources.size(), 0.0);

            for (int tIx = 0; tIx < nTasks; ++tIx) {
                const auto& t = I.tasks[tIx];
                const int req = std::max(0, t.totalRequiredLevel());
                if (req <= 0) continue;

                int feasibleCount = 0;
                double cheapest = std::numeric_limits<double>::infinity();
                double second = std::numeric_limits<double>::infinity();

                if (!t.capableResourceIndices.empty()) {
                    feasibleCount = (int)t.capableResourceIndices.size();

                    for (int ri : t.capableResourceIndices) {
                        if (ri < 0 || ri >= (int)I.resources.size()) continue;

                        const double sal = I.resources[ri].salary;
                        if (sal < cheapest) {
                            second = cheapest;
                            cheapest = sal;
                        }
                        else if (sal < second) {
                            second = sal;
                        }
                    }
                }
                else {
                    for (int ri = 0; ri < (int)I.resources.size(); ++ri) {
                        const auto& rr = I.resources[ri];
                        if (!t.canBeDoneBy(rr)) continue;

                        ++feasibleCount;
                        const double sal = rr.salary;
                        if (sal < cheapest) {
                            second = cheapest;
                            cheapest = sal;
                        }
                        else if (sal < second) {
                            second = sal;
                        }
                    }
                }

                if (feasibleCount <= 0 || !std::isfinite(cheapest)) continue;
                if (!std::isfinite(second)) second = cheapest;

                double critNorm = 0.0;
                double slackNorm = 0.0;
                double descNorm = 0.0;

                if (hasCPM) {
                    critNorm = std::min(1.0, (double)cpm.critLen[tIx] / std::max(1.0, projHorizon));
                    slackNorm = std::min(1.0, std::max(0.0, (double)cpm.slack[tIx]) / std::max(1.0, projHorizon));
                    descNorm = std::min(1.0, (double)cpm.descCount[tIx] / std::max(1.0, (double)nTasks));
                }

                const double scarcity = (double)req / (double)std::max(1, feasibleCount);
                const double structural = (1.0 + critNorm + 0.5 * descNorm) / (1.0 + slackNorm);
                const double baseValue = (double)t.duration * scarcity * structural;

                if (!t.capableResourceIndices.empty()) {
                    for (int ri : t.capableResourceIndices) {
                        if (ri < 0 || ri >= (int)I.resources.size()) continue;

                        const double cheapAdv = std::max(0.0, second - I.resources[ri].salary);
                        futureCheapnessByResTmp[ri] += baseValue * cheapAdv;
                    }
                }
                else {
                    for (int ri = 0; ri < (int)I.resources.size(); ++ri) {
                        const auto& rr = I.resources[ri];
                        if (!t.canBeDoneBy(rr)) continue;

                        const double cheapAdv = std::max(0.0, second - rr.salary);
                        futureCheapnessByResTmp[ri] += baseValue * cheapAdv;
                    }
                }
            }

            if (req <= 0) {
                for (int ri = 0; ri < (int)I.resources.size(); ++ri) {
                    updateMisuseFor(ri);
                }
            }
            else if (!t.capableResourceIndices.empty()) {
                for (int ri : t.capableResourceIndices) {
                    updateMisuseFor(ri);
                }
            }
            else {
                for (int ri = 0; ri < (int)I.resources.size(); ++ri) {
                    const auto& rr = I.resources[ri];
                    if (!t.canBeDoneBy(rr)) continue;
                    updateMisuseFor(ri);
                }
            }
        }

        s.maxMinFeasibleCostNow = std::max(1.0, maxMinFeasibleCostNow);
        s.maxCostRegretNow = std::max(1.0, maxCostRegretNow);
        s.maxResWagePerLevel = std::max(1.0, maxResWagePerLevel);

        s.maxResSurplusLevel = std::max(1.0, maxResSurplusLevel);
        s.maxResRelativeWage = std::max(1.0, maxResRelativeWage);
        s.maxResReservePressure = std::max(1.0, maxResReservePressure);
        s.maxResCriticalReserve = std::max(1.0, maxResCriticalReserve);
        s.maxResBottleneckPreservation = std::max(1.0, maxResBottleneckPreservation);
        s.maxResSpecialistMisuse = std::max(1.0, maxResSpecialistMisuse);

        s.maxWaitRes = std::max(1.0, projHorizon);
        s.maxEstPrec = std::max(1.0, projHorizon);
        s.maxCritLen = s.maxEstPrec;
        s.maxSlackPos = s.maxEstPrec;
        s.maxCriticalPressure = s.maxCritLen;

        s.maxSuccCount = s.maxNumTasks;
        s.maxDescCount = s.maxNumTasks;
        s.maxTaskReleasePressure = std::max(1.0, maxTaskReleasePressure);
        s.maxTotPred = s.maxNumTasks;

        g_scaling = s;
    }

    const FeatureScaling& getFeatureScaling() {
        return g_scaling;
    }

} // namespace gp