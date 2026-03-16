#include "Features.hpp"
#include "FeatureScaling.hpp"
#include <algorithm>
#include <limits>
#include <cmath>
#include "Precompute.hpp"
#include "../alloc/ResourceAllocator.hpp"

namespace {

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

    inline int countUnschedTasks(const Instance& I) {
        int unsched = 0;
        for (const auto& tt : I.tasks) if (tt.start < 0) ++unsched;
        return unsched;
    }

    inline double taskResCountFeature(const Instance& I, const Task& t, int req) {
        if (req <= 0) return 1.0;

        for (const auto& r : I.resources) {
            auto it = r.skills.find(t.reqSkill);
            int lvl = (it != r.skills.end()) ? it->second : 0;
            if (lvl >= req) return 1.0;
        }
        return inf();
    }

    inline double avgSalaryForSkill(const Instance& I, const Task& t, int req) {
        if (req <= 0) return inf();

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

    inline void fillCostNowFeatures(Features& f, const Instance& I, const Task& t, int req, int now) {
        if (!f.feasibleNow) {
            f.cheapestCostNow = inf();
            f.costPerSkillNow = inf();
            f.teamSizeMinNow = inf();
            f.minWageAvail = inf();
            f.avgWageAvail = inf();
            return;
        }

        auto pick = ResourceAllocator::cheapestSubset(I, t.reqSkill, req, now);
        if (!pick) {
            f.cheapestCostNow = inf();
            f.costPerSkillNow = inf();
            f.teamSizeMinNow = inf();
            f.minWageAvail = inf();
            f.avgWageAvail = inf();
            return;
        }

        f.teamSizeMinNow = (double)pick->size();
        f.cheapestCostNow = ResourceAllocator::subsetCost(I, *pick);
        f.costPerSkillNow = (req > 0) ? (f.cheapestCostNow / (double)req) : f.cheapestCostNow;

        double sum = 0.0;
        double mn = inf();

        for (int rid : *pick) {
            auto it = std::find_if(I.resources.begin(), I.resources.end(),
                [&](const Resource& x) { return x.id == rid; });
            if (it != I.resources.end()) {
                mn = std::min(mn, it->salary);
                sum += it->salary;
            }
        }

        if (!pick->empty() && std::isfinite(mn)) {
            f.minWageAvail = mn;
            f.avgWageAvail = sum / (double)pick->size();
        }
        else {
            f.minWageAvail = inf();
            f.avgWageAvail = inf();
        }
    }

    inline void normalizeTaskFeatures(Features& f, const gp::FeatureScaling& S) {
        f.duration = normalize(f.duration, S.maxDuration);
        f.reqLevel = normalize(f.reqLevel, S.maxReqLevel);
        f.numTasks = normalize(f.numTasks, S.maxNumTasks);
        f.numResources = normalize(f.numResources, S.maxNumResources);
        f.numSkills = normalize(f.numSkills, S.maxNumSkills);
        f.taskResCount = normalize(f.taskResCount, S.maxTeamSizeMinNow);
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
    }

} // namespace



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

    f.taskResCount = taskResCountFeature(I, t, req);
    f.avgResCostForSkill = avgSalaryForSkill(I, t, req);
    f.unschedTasks = (double)countUnschedTasks(I);

    const bool predsDone = predecessorsDoneNow(I, t, ctx.now);

    f.availSkill = ResourceAllocator::availableSkillSum(I, ctx.now, t.reqSkill);
    f.availGap = f.availSkill - (double)req;
    f.waitRes = ResourceAllocator::waitUntilFeasible(I, ctx.now, t.reqSkill, req);
    f.feasibleNow = predsDone && (f.waitRes <= 0.0);

    if (auto cpm = gp::getCPMPrecalc()) {
        f.estPrec = cpm->est[taskIx];
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

    {
        auto it = r.skills.find(t.reqSkill);
        f.resSkillLevel = (it != r.skills.end()) ? (double)it->second : 0.0;
    }

    f.resFreeTime = std::max(0.0, (double)(r.busyUntil - now));
    f.resMultiSkill = (double)r.skills.size();
    f.resUtilization = (r.busyUntil > now) ? 1.0 : 0.0;

    const auto& S = gp::getFeatureScaling();
    f.resWage = normalize(f.resWage, S.maxMinWageAvail);
    f.resSkillLevel = normalize(f.resSkillLevel, S.maxResSkillLevel);
    f.resFreeTime = normalize(f.resFreeTime, S.maxWaitRes);
    f.resMultiSkill = normalize(f.resMultiSkill, S.maxNumSkills);
    f.resUtilization = normalize(f.resUtilization, 1.0);

    return f;
}


