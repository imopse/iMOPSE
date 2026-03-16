#include "Features.hpp"
#include "FeatureScaling.hpp"
#include <algorithm>
#include <limits>
#include <cmath>
#include "Precompute.hpp"
#include "../alloc/ResourceAllocator.hpp"

namespace {

    const std::vector<double>* g_taskResCountByTask = nullptr;
    const std::vector<double>* g_avgResCostByTask = nullptr;
    const std::unordered_map<int, int>* g_resIndexById = nullptr;
    const int* g_unschedCountPtr = nullptr;
    const std::vector<int>* g_remainingPredCountByTask = nullptr;
    const std::vector<int>* g_latestPredFinishByTask = nullptr;
    const std::unordered_map<std::string, SkillStepInfo>* g_skillStepCache = nullptr;

    inline double inf() { return std::numeric_limits<double>::infinity(); }

    inline double normalize(double val, double maxVal) {
        if (!std::isfinite(val)) return 1.0;
        if (maxVal <= 0.0)       return 0.0;
        double x = val / maxVal;
        if (x < 0.0) x = 0.0;
        if (x > 1.0) x = 1.0;
        return x;
    }

    inline bool predecessorsDoneNow(const Instance& I, const Task& t, int now) {
        for (int pid : t.predecessors) {
            auto it = I.idToIndex.find(pid);
            if (it != I.idToIndex.end()) {
                const Task& p = I.tasks[it->second];
                if (p.finish < 0 || p.finish > now) return false;
            }
        }
        return true;
    }

    inline double latestPredFinish(const Instance& I, const Task& t) {
        int mx = 0;
        for (int pid : t.predecessors) {
            auto it = I.idToIndex.find(pid);
            if (it != I.idToIndex.end()) {
                const Task& p = I.tasks[it->second];
                if (p.finish > mx) mx = p.finish;
            }
        }
        return (double)mx;
    }

    inline int countUnschedTasks(const Instance& I) {
        if (g_unschedCountPtr) return *g_unschedCountPtr;

        int unsched = 0;
        for (const auto& tt : I.tasks) if (tt.start < 0) ++unsched;
        return unsched;
    }

    inline double taskResCountFeature(const Instance& I, int taskIx, const Task& t, int req) {
        if (g_taskResCountByTask && taskIx >= 0 && taskIx < (int)g_taskResCountByTask->size()) {
            return (*g_taskResCountByTask)[taskIx];
        }

        if (req <= 0) return (double)I.resources.size();

        if (!t.capableResources.empty()) {
            return (double)t.capableResources.size();
        }

        int cnt = 0;
        for (const auto& r : I.resources) {
            auto it = r.skills.find(t.reqSkill);
            int lvl = (it != r.skills.end()) ? it->second : 0;
            if (lvl >= req) ++cnt;
        }
        return (double)cnt;
    }

    inline double avgSalaryForSkill(const Instance& I, int taskIx, const Task& t, int req) {
        if (g_avgResCostByTask && taskIx >= 0 && taskIx < (int)g_avgResCostByTask->size()) {
            return (*g_avgResCostByTask)[taskIx];
        }

        if (req <= 0) return inf();

        if (!t.capableResourceIndices.empty()) {
            double sumSal = 0.0;
            int cnt = 0;
            for (int ri : t.capableResourceIndices) {
                if (ri < 0 || ri >= (int)I.resources.size()) continue;
                sumSal += I.resources[ri].salary;
                ++cnt;
            }
            return (cnt > 0) ? (sumSal / (double)cnt) : inf();
        }

        double sumSal = 0.0;
        int cnt = 0;
        for (const auto& r : I.resources) {
            auto it = r.skills.find(t.reqSkill);
            if (it != r.skills.end() && it->second >= req) {
                sumSal += r.salary;
                ++cnt;
            }
        }
        return (cnt > 0) ? (sumSal / (double)cnt) : inf();
    }

    inline const SkillStepInfo* skillStepInfoFor(const std::string& skill) {
        if (!g_skillStepCache) return nullptr;
        auto it = g_skillStepCache->find(skill);
        if (it == g_skillStepCache->end()) return nullptr;
        return &it->second;
    }

    inline double cachedAvailSkill(const std::string& skill) {
        const SkillStepInfo* info = skillStepInfoFor(skill);
        if (!info) return -1.0;
        return (double)info->maxFreeLevel;
    }

    inline double cachedWaitRes(const std::string& skill, int req) {
        const SkillStepInfo* info = skillStepInfoFor(skill);
        if (!info || req <= 0) return -1.0;
        if (req >= (int)info->minWaitAtLeast.size()) return (double)std::numeric_limits<int>::max();
        return (double)info->minWaitAtLeast[req];
    }

    inline bool cachedCheapestPair(const std::string& skill, int req, double& first, double& second) {
        const SkillStepInfo* info = skillStepInfoFor(skill);
        if (!info || req <= 0) return false;
        if (req >= (int)info->cheapestAtLeast.size()) return false;

        first = info->cheapestAtLeast[req];
        second = info->secondCheapestAtLeast[req];

        return std::isfinite(first);
    }


    inline void fillCostNowFeatures(Features& f, const Instance& I, const Task& t, int req, int now) {
        if (!f.feasibleNow) {
            f.cheapestCostNow = inf();
            f.costPerSkillNow = inf();
            f.teamSizeMinNow = inf();
            f.minWageAvail = inf();
            f.avgWageAvail = inf();
            f.minFeasibleCostNow = inf();
            f.costRegretNow = 0.0;
            return;
        }

        if (t.capableResources.empty() && req > 0 && !t.reqSkill.empty()) {
            double first = std::numeric_limits<double>::infinity();
            double second = std::numeric_limits<double>::infinity();

            if (cachedCheapestPair(t.reqSkill, req, first, second)) {
                if (!std::isfinite(second)) second = first;

                f.teamSizeMinNow = 1.0;
                f.cheapestCostNow = first;
                f.costPerSkillNow = first / (double)req;
                f.minFeasibleCostNow = first * (double)t.duration;
                f.costRegretNow = (second - first) * (double)t.duration;
                f.minWageAvail = first;
                f.avgWageAvail = first;
                return;
            }
        }


        double first = std::numeric_limits<double>::infinity();
        double second = std::numeric_limits<double>::infinity();
        bool found = false;

        auto considerSalary = [&](double sal) {
            found = true;
            if (sal < first) {
                second = first;
                first = sal;
            }
            else if (sal < second) {
                second = sal;
            }
            };

        if (!t.capableResourceIndices.empty()) {
            for (int ri : t.capableResourceIndices) {
                if (ri < 0 || ri >= (int)I.resources.size()) continue;

                const auto& r = I.resources[ri];
                if (r.busyUntil > now) continue;

                considerSalary(r.salary);
            }
        }
        else {
            for (const auto& r : I.resources) {
                if (r.busyUntil > now) continue;

                int lvl = 0;
                auto it = r.skills.find(t.reqSkill);
                if (it != r.skills.end()) lvl = it->second;

                if (req <= 0 || lvl >= req) {
                    considerSalary(r.salary);
                }
            }
        }

        if (!found || !std::isfinite(first)) {
            f.cheapestCostNow = inf();
            f.costPerSkillNow = inf();
            f.teamSizeMinNow = inf();
            f.minWageAvail = inf();
            f.avgWageAvail = inf();
            f.minFeasibleCostNow = inf();
            f.costRegretNow = 0.0;
            return;
        }

        if (!std::isfinite(second)) second = first;

        f.teamSizeMinNow = 1.0;
        f.cheapestCostNow = first;
        f.costPerSkillNow = (req > 0) ? (first / (double)req) : first;
        f.minFeasibleCostNow = first * (double)t.duration;
        f.costRegretNow = (second - first) * (double)t.duration;
        f.minWageAvail = first;
        f.avgWageAvail = first;
    }

    inline void normalizeTaskFeatures(Features& f, const gp::FeatureScaling& S) {
        f.duration = normalize(f.duration, S.maxDuration);
        f.reqLevel = normalize(f.reqLevel, S.maxReqLevel);
        f.numTasks = normalize(f.numTasks, S.maxNumTasks);
        f.numResources = normalize(f.numResources, S.maxNumResources);
        f.numSkills = normalize(f.numSkills, S.maxNumSkills);
        f.taskResCount = normalize(f.taskResCount, S.maxTaskResCount);
        f.avgResCostForSkill = normalize(f.avgResCostForSkill, S.maxAvgResCostForSkill);
        f.unschedTasks = normalize(f.unschedTasks, S.maxUnschedTasks);
        f.availSkill = normalize(f.availSkill, S.maxAvailSkill);

        const double minGap = -S.maxReqLevel;
        const double maxGap = S.maxAvailGapPos;
        if (!std::isfinite(f.availGap)) {
            f.availGap = 1.0;
        }
        else {
            double x = (f.availGap - minGap) / (maxGap - minGap);
            if (x < 0.0) x = 0.0;
            if (x > 1.0) x = 1.0;
            f.availGap = x;
        }

        f.waitRes = normalize(f.waitRes, S.maxWaitRes);
        f.estPrec = normalize(f.estPrec, S.maxEstPrec);
        f.critLen = normalize(f.critLen, S.maxCritLen);
        f.slack = normalize(f.slack, S.maxSlackPos);
        f.succCount = normalize(f.succCount, S.maxSuccCount);
        f.totPred = normalize(f.totPred, S.maxTotPred);

        f.cheapestCostNow = normalize(f.cheapestCostNow, S.maxCheapestCostNow);
        f.costPerSkillNow = normalize(f.costPerSkillNow, S.maxCostPerSkillNow);
        f.teamSizeMinNow = normalize(f.teamSizeMinNow, S.maxTeamSizeMinNow);
        f.minWageAvail = normalize(f.minWageAvail, S.maxMinWageAvail);
        f.avgWageAvail = normalize(f.avgWageAvail, S.maxAvgWageAvail);

        f.minFeasibleCostNow = normalize(f.minFeasibleCostNow, S.maxMinFeasibleCostNow);
        f.costRegretNow = normalize(f.costRegretNow, S.maxCostRegretNow);
    }

} // namespace

void setFeaturePrecomputed(
    const std::vector<double>* taskResCountByTask,
    const std::vector<double>* avgResCostByTask,
    const std::unordered_map<int, int>* resIndexById,
    const int* unschedCountPtr,
    const std::vector<int>* remainingPredCountByTask,
    const std::vector<int>* latestPredFinishByTask,
    const std::unordered_map<std::string, SkillStepInfo>* skillStepCache)
{
    g_taskResCountByTask = taskResCountByTask;
    g_avgResCostByTask = avgResCostByTask;
    g_resIndexById = resIndexById;
    g_unschedCountPtr = unschedCountPtr;
    g_remainingPredCountByTask = remainingPredCountByTask;
    g_latestPredFinishByTask = latestPredFinishByTask;
    g_skillStepCache = skillStepCache;
}


void clearFeaturePrecomputed() {
    g_taskResCountByTask = nullptr;
    g_avgResCostByTask = nullptr;
    g_resIndexById = nullptr;
    g_unschedCountPtr = nullptr;
    g_remainingPredCountByTask = nullptr;
    g_latestPredFinishByTask = nullptr;
    g_skillStepCache = nullptr;
}


Features computeFeatures(const PriorityContext& ctx, int taskIx) {
    Features f{};
    const Instance& I = *ctx.inst;
    const Task& t = I.tasks[taskIx];

    f.duration = t.duration;
    f.reqLevel = t.reqLevel;
    const int req = std::max(0, t.reqLevel);

    const auto& S = gp::getFeatureScaling();

    f.numTasks = S.maxNumTasks;
    f.numResources = S.maxNumResources;
    f.numSkills = S.maxNumSkills;

    f.taskResCount = taskResCountFeature(I, taskIx, t, req);
    f.avgResCostForSkill = avgSalaryForSkill(I, taskIx, t, req);
    f.unschedTasks = (double)countUnschedTasks(I);

    bool predsDone = false;
    if (g_remainingPredCountByTask &&
        taskIx >= 0 &&
        taskIx < (int)g_remainingPredCountByTask->size()) {
        predsDone = ((*g_remainingPredCountByTask)[taskIx] == 0);
    }
    else {
        predsDone = predecessorsDoneNow(I, t, ctx.now);
    }

    double availCached = -1.0;
    double waitCached = -1.0;

    if (req > 0 && !t.reqSkill.empty()) {
        availCached = cachedAvailSkill(t.reqSkill);
        waitCached = cachedWaitRes(t.reqSkill, req);
    }

    f.availSkill = (availCached >= 0.0)
        ? availCached
        : (double)ResourceAllocator::availableSkillSum(I, ctx.now, t.reqSkill);

    f.availGap = f.availSkill - (double)req;

    f.waitRes = (waitCached >= 0.0)
        ? waitCached
        : (double)ResourceAllocator::waitUntilFeasible(I, ctx.now, t.reqSkill, req);

    f.feasibleNow = predsDone && (f.waitRes <= 0.0);


    if (g_latestPredFinishByTask &&
        taskIx >= 0 &&
        taskIx < (int)g_latestPredFinishByTask->size()) {
        f.estPrec = (double)(*g_latestPredFinishByTask)[taskIx];
    }
    else {
        f.estPrec = latestPredFinish(I, t);
    }

    if (auto cpm = gp::getCPMPrecalc()) {
        f.critLen = cpm->critLen[taskIx];
        f.slack = cpm->slack[taskIx];
        f.succCount = cpm->succCount[taskIx];
        f.totPred = cpm->totPred[taskIx];
    }

    fillCostNowFeatures(f, I, t, req, ctx.now);
    normalizeTaskFeatures(f, S);

    return f;
}


Features computeResourceFeatures(const Instance& I, const Task& t, const Resource& r, int now) {
    Features f{};

    f.resWage = r.salary;

    int req = std::max(0, t.reqLevel);
    int lvlNow = 0;
    {
        auto it = r.skills.find(t.reqSkill);
        lvlNow = (it != r.skills.end()) ? it->second : 0;
        f.resSkillLevel = (double)lvlNow;
    }

    f.resWaitTime = (r.busyUntil > now) ? (double)(r.busyUntil - now) : 0.0;
    f.resIdleTime = (r.busyUntil < now) ? (double)(now - r.busyUntil) : 0.0;
    f.resCanStartNow = (r.busyUntil <= now) ? 1.0 : 0.0;

    f.resMultiSkill = (double)r.skills.size();

    double busySoFar = (double)r.totalBusy;
    if (r.busy && now > r.busyStart) {
        busySoFar += (double)(now - r.busyStart);
    }
    f.resUtilization = (now > 0) ? (busySoFar / (double)now) : 0.0;

    f.resWagePerLevel = r.salary / (double)std::max(1, lvlNow);
    f.resAssignCost = r.salary * (double)t.duration;

    f.resSurplusLevel = (double)std::max(0, lvlNow - req);

    double cheapestNow = std::numeric_limits<double>::infinity();
    double cheapestCapableOverall = std::numeric_limits<double>::infinity();
    double waitOfCheapestCapableOverall = std::numeric_limits<double>::infinity();

    for (const auto& rr : I.resources) {
        int lvl = 0;
        auto it = rr.skills.find(t.reqSkill);
        if (it != rr.skills.end()) lvl = it->second;

        if (req > 0 && lvl < req) continue;

        const double waitRR = (rr.busyUntil > now) ? (double)(rr.busyUntil - now) : 0.0;

        if (rr.salary < cheapestCapableOverall) {
            cheapestCapableOverall = rr.salary;
            waitOfCheapestCapableOverall = waitRR;
        }
        else if (rr.salary == cheapestCapableOverall && waitRR < waitOfCheapestCapableOverall) {
            waitOfCheapestCapableOverall = waitRR;
        }

        if (rr.busyUntil <= now) {
            cheapestNow = std::min(cheapestNow, rr.salary);
        }
    }

    f.resRelativeWage = std::isfinite(cheapestNow) ? (r.salary - cheapestNow) : 0.0;

    double taskSlack = 0.0;
    if (auto cpm = gp::getCPMPrecalc()) {
        auto itIx = I.idToIndex.find(t.id);
        if (itIx != I.idToIndex.end()) {
            const int ix = itIx->second;
            if (ix >= 0 && ix < (int)cpm->slack.size()) {
                taskSlack = (double)cpm->slack[ix];
            }
        }
    }

    const double waitThis = f.resWaitTime;
    const double waitSavedVsCheapest =
        std::isfinite(waitOfCheapestCapableOverall)
        ? std::max(0.0, waitOfCheapestCapableOverall - waitThis)
        : 0.0;

    const double usefulWaitSaved = std::max(0.0, waitSavedVsCheapest - taskSlack);

    const double premiumCost =
        std::isfinite(cheapestCapableOverall)
        ? std::max(0.0, r.salary - cheapestCapableOverall) * (double)t.duration
        : 0.0;

    f.resHasteValue = premiumCost / (1.0 + usefulWaitSaved);
    f.resAssignPremiumAll =
        std::isfinite(cheapestCapableOverall)
        ? std::max(0.0, r.salary - cheapestCapableOverall) * (double)t.duration
        : 0.0;

    double demand = 0.0;
    double reservePressure = 0.0;
    std::unordered_map<std::string, double> familyPressure;

    for (const auto& u : I.tasks) {
        if (u.start != -1) continue;
        if (u.id == t.id) continue;

        int reqU = std::max(0, u.reqLevel);

        int lvlU = 0;
        auto it = r.skills.find(u.reqSkill);
        if (it != r.skills.end()) lvlU = it->second;

        if (reqU > 0 && lvlU < reqU) continue;

        int feasibleCount = 0;
        double cheapest = std::numeric_limits<double>::infinity();
        double second = std::numeric_limits<double>::infinity();

        for (const auto& rr : I.resources) {
            auto jt = rr.skills.find(u.reqSkill);
            int ll = (jt != rr.skills.end()) ? jt->second : 0;
            if (reqU > 0 && ll < reqU) continue;

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

        const double reserveContribution =
            ((double)u.duration * priceGap) / (double)std::max(1, feasibleCount);

        reservePressure += reserveContribution;

        const std::string famKey =
            u.reqSkill + "#" + std::to_string(std::max(0, u.reqLevel));
        familyPressure[famKey] += reserveContribution;
    }

    f.resFutureDemand = demand;
    f.resReservePressure = reservePressure;

    const std::string currentFamKey =
        t.reqSkill + "#" + std::to_string(std::max(0, t.reqLevel));

    double currentFamilyPressure = 0.0;
    auto itFam = familyPressure.find(currentFamKey);
    if (itFam != familyPressure.end()) {
        currentFamilyPressure = itFam->second;
    }

    double bestOtherFamilyPressure = 0.0;
    for (const auto& kv : familyPressure) {
        if (kv.first == currentFamKey) continue;
        if (kv.second > bestOtherFamilyPressure) {
            bestOtherFamilyPressure = kv.second;
        }
    }

    f.resFamilyMismatch =
        bestOtherFamilyPressure / (1.0 + currentFamilyPressure);

    const auto& S = gp::getFeatureScaling();

    double critRaw = 0.0;
    double slackRaw = 0.0;
    if (auto cpm = gp::getCPMPrecalc()) {
        auto itIx = I.idToIndex.find(t.id);
        if (itIx != I.idToIndex.end()) {
            const int ix = itIx->second;
            if (ix >= 0 && ix < (int)cpm->critLen.size()) critRaw = cpm->critLen[ix];
            if (ix >= 0 && ix < (int)cpm->slack.size())   slackRaw = cpm->slack[ix];
        }
    }

    const double critNorm =
        (S.maxCritLen > 0.0) ? (critRaw / S.maxCritLen) : 0.0;
    const double slackNorm =
        (S.maxSlackPos > 0.0) ? (std::max(0.0, slackRaw) / S.maxSlackPos) : 0.0;

    const double currentNeed =
        ((double)t.duration * (1.0 + critNorm)) / (1.0 + slackNorm);

    f.resStrategicMismatch =
        premiumCost * (1.0 + f.resReservePressure) / (1.0 + currentNeed);
    f.resWage = normalize(f.resWage, S.maxMinWageAvail);
    f.resSkillLevel = normalize(f.resSkillLevel, S.maxResSkillLevel);
    f.resWaitTime = normalize(f.resWaitTime, S.maxWaitRes);
    f.resIdleTime = normalize(f.resIdleTime, S.maxWaitRes);
    f.resCanStartNow = (f.resCanStartNow > 0.5) ? 1.0 : 0.0;
    f.resMultiSkill = normalize(f.resMultiSkill, S.maxNumSkills);
    f.resUtilization = normalize(f.resUtilization, 1.0);
    f.resWagePerLevel = normalize(f.resWagePerLevel, S.maxResWagePerLevel);
    f.resHasteValue = normalize(f.resHasteValue, S.maxMinWageAvail * S.maxDuration);
    f.resAssignCost = normalize(f.resAssignCost, S.maxMinWageAvail * S.maxDuration);
    f.resAssignPremiumAll = normalize(f.resAssignPremiumAll, S.maxMinWageAvail * S.maxDuration);
    f.resStrategicMismatch = normalize(
        f.resStrategicMismatch,
        (S.maxMinWageAvail * S.maxDuration) * (1.0 + S.maxResReservePressure)
    );
    f.resFamilyMismatch = normalize(f.resFamilyMismatch, S.maxResReservePressure);

    f.resSurplusLevel = normalize(f.resSurplusLevel, S.maxResSurplusLevel);
    f.resRelativeWage = normalize(f.resRelativeWage, S.maxResRelativeWage);
    f.resFutureDemand = normalize(f.resFutureDemand, S.maxResFutureDemand);
    f.resReservePressure = normalize(f.resReservePressure, S.maxResReservePressure);

    return f;
}


Features computeResourceFeaturesFast(
    const Instance& I,
    int taskIx,
    const Task& t,
    const Resource& r,
    int now,
    double cheapestNow,
    double cheapestCapableOverall,
    double waitOfCheapestCapableOverall,
    double futureDemandExcludingTask,
    double reservePressureExcludingTask,
    double familyMismatchExcludingTask
) {
    Features f{};

    f.resWage = r.salary;

    const int req = std::max(0, t.reqLevel);

    int lvlNow = 0;
    {
        auto it = r.skills.find(t.reqSkill);
        lvlNow = (it != r.skills.end()) ? it->second : 0;
        f.resSkillLevel = (double)lvlNow;
    }

    f.resWaitTime = (r.busyUntil > now) ? (double)(r.busyUntil - now) : 0.0;
    f.resIdleTime = (r.busyUntil < now) ? (double)(now - r.busyUntil) : 0.0;
    f.resCanStartNow = (r.busyUntil <= now) ? 1.0 : 0.0;

    f.resMultiSkill = (double)r.skills.size();

    double busySoFar = (double)r.totalBusy;
    if (r.busy && now > r.busyStart) busySoFar += (double)(now - r.busyStart);
    f.resUtilization = (now > 0) ? (busySoFar / (double)now) : 0.0;

    f.resWagePerLevel = r.salary / (double)std::max(1, lvlNow);
    f.resAssignCost = r.salary * (double)t.duration;
    f.resSurplusLevel = (double)std::max(0, lvlNow - req);

    f.resRelativeWage = std::isfinite(cheapestNow) ? (r.salary - cheapestNow) : 0.0;
    f.resAssignPremiumAll =
        std::isfinite(cheapestCapableOverall)
        ? std::max(0.0, r.salary - cheapestCapableOverall) * (double)t.duration
        : 0.0;

    double taskSlack = 0.0;
    if (auto cpm = gp::getCPMPrecalc()) {
        if (taskIx >= 0 && taskIx < (int)cpm->slack.size()) {
            taskSlack = (double)cpm->slack[taskIx];
        }
    }

    const double waitThis = f.resWaitTime;
    const double waitSavedVsCheapest =
        std::isfinite(waitOfCheapestCapableOverall)
        ? std::max(0.0, waitOfCheapestCapableOverall - waitThis)
        : 0.0;

    const double usefulWaitSaved = std::max(0.0, waitSavedVsCheapest - taskSlack);

    const double premiumCost =
        std::isfinite(cheapestCapableOverall)
        ? std::max(0.0, r.salary - cheapestCapableOverall) * (double)t.duration
        : 0.0;

    f.resHasteValue = premiumCost / (1.0 + usefulWaitSaved);

    f.resFutureDemand = futureDemandExcludingTask;
    f.resReservePressure = reservePressureExcludingTask;
    f.resFamilyMismatch = familyMismatchExcludingTask;

    const auto& S = gp::getFeatureScaling();

    double critRaw = 0.0;
    double slackRaw = 0.0;
    if (auto cpm = gp::getCPMPrecalc()) {
        if (taskIx >= 0 && taskIx < (int)cpm->critLen.size()) critRaw = cpm->critLen[taskIx];
        if (taskIx >= 0 && taskIx < (int)cpm->slack.size())   slackRaw = cpm->slack[taskIx];
    }

    const double critNorm =
        (S.maxCritLen > 0.0) ? (critRaw / S.maxCritLen) : 0.0;
    const double slackNorm =
        (S.maxSlackPos > 0.0) ? (std::max(0.0, slackRaw) / S.maxSlackPos) : 0.0;

    const double currentNeed =
        ((double)t.duration * (1.0 + critNorm)) / (1.0 + slackNorm);

    f.resStrategicMismatch =
        premiumCost * (1.0 + f.resReservePressure) / (1.0 + currentNeed);

    (void)I;
    f.resWage = normalize(f.resWage, S.maxMinWageAvail);
    f.resSkillLevel = normalize(f.resSkillLevel, S.maxResSkillLevel);
    f.resWaitTime = normalize(f.resWaitTime, S.maxWaitRes);
    f.resIdleTime = normalize(f.resIdleTime, S.maxWaitRes);
    f.resCanStartNow = (f.resCanStartNow > 0.5) ? 1.0 : 0.0;
    f.resMultiSkill = normalize(f.resMultiSkill, S.maxNumSkills);
    f.resUtilization = normalize(f.resUtilization, 1.0);
    f.resWagePerLevel = normalize(f.resWagePerLevel, S.maxResWagePerLevel);
    f.resHasteValue = normalize(f.resHasteValue, S.maxMinWageAvail * S.maxDuration);
    f.resAssignCost = normalize(f.resAssignCost, S.maxMinWageAvail * S.maxDuration);
    f.resAssignPremiumAll = normalize(f.resAssignPremiumAll, S.maxMinWageAvail * S.maxDuration);
    f.resStrategicMismatch = normalize(
        f.resStrategicMismatch,
        (S.maxMinWageAvail * S.maxDuration) * (1.0 + S.maxResReservePressure)
    );
    f.resFamilyMismatch = normalize(f.resFamilyMismatch, S.maxResReservePressure);

    f.resSurplusLevel = normalize(f.resSurplusLevel, S.maxResSurplusLevel);
    f.resRelativeWage = normalize(f.resRelativeWage, S.maxResRelativeWage);
    f.resFutureDemand = normalize(f.resFutureDemand, S.maxResFutureDemand);
    f.resReservePressure = normalize(f.resReservePressure, S.maxResReservePressure);

    return f;
}