#include "Features.hpp"
#include <algorithm>
#include <limits>
#include <unordered_set> 
#include <unordered_map>
#include <string> 
#include <cmath>
#include "Precompute.hpp"
#include "../alloc/ResourceAllocator.hpp"



Features computeFeatures(const PriorityContext& ctx, int taskIx) {
    Features f{};
    const Instance& I = *ctx.inst;
    const Task& t = I.tasks[taskIx];

    f.duration = t.duration;
    f.reqLevel = t.reqLevel;
    const int req = std::max(0, t.reqLevel);

    f.numTasks = (double)I.tasks.size();
    f.numResources = (double)I.resources.size();

    {
        std::unordered_set<std::string> allSkills;
        for (const auto& r : I.resources) {
            for (const auto& kv : r.skills) {
                allSkills.insert(kv.first);
            }
        }
        f.numSkills = (double)allSkills.size();
    }


    {
        if (req <= 0) {
            f.taskResCount = 1.0;
        }
        else {
            bool exists = false;
            for (const auto& r : I.resources) {
                auto it = r.skills.find(t.reqSkill);
                int lvl = (it != r.skills.end()) ? it->second : 0;
                if (lvl >= req) { exists = true; break; }
            }
            f.taskResCount = exists ? 1.0 : std::numeric_limits<double>::infinity();
        }
    }


    {
        double sumSal = 0.0;
        int    cnt = 0;
        for (const auto& r : I.resources) {
            auto it = r.skills.find(t.reqSkill);
            if (it != r.skills.end() && it->second >= req)
            {
                sumSal += r.salary;
                ++cnt;
            }
        }
        if (cnt > 0) {
            f.avgResCostForSkill = sumSal / (double)cnt;
        }
        else {
            f.avgResCostForSkill = std::numeric_limits<double>::infinity();
        }
    }


    {
        int unsched = 0;
        for (const auto& tt : I.tasks) {
            if (tt.start < 0) ++unsched;
        }
        f.unschedTasks = (double)unsched;
    }


    bool predsDone = true;
    for (int pid : t.predecessors) {
        auto it = I.idToIndex.find(pid);
        if (it != I.idToIndex.end()) {
            const Task& p = I.tasks[it->second];
            if (p.finish < 0 || p.finish > ctx.now) { predsDone = false; break; }
        }
    }

    double avail = ResourceAllocator::availableSkillSum(I, ctx.now, t.reqSkill);
    f.availSkill = avail;
    f.availGap = avail - (double)req;
    f.waitRes = ResourceAllocator::waitUntilFeasible(I, ctx.now, t.reqSkill, req);
    f.feasibleNow = predsDone && (f.waitRes <= 0.0);


    if (auto cpm = gp::getCPMPrecalc()) {
        f.estPrec = cpm->est[taskIx];
        f.critLen = cpm->critLen[taskIx];
        f.slack = cpm->slack[taskIx];
        f.succCount = cpm->succCount[taskIx];
        f.totPred = cpm->totPred[taskIx];
    }
    else {
    }

    if (!f.feasibleNow) {
        f.cheapestCostNow = std::numeric_limits<double>::infinity();
        f.costPerSkillNow = std::numeric_limits<double>::infinity();
        f.teamSizeMinNow = std::numeric_limits<double>::infinity();
        f.minWageAvail = std::numeric_limits<double>::infinity();
        f.avgWageAvail = std::numeric_limits<double>::infinity();
    }
    else {
        if (auto pick = ResourceAllocator::cheapestSubset(I, t.reqSkill, req, ctx.now)) {
            f.teamSizeMinNow = (double)pick->size();
            f.cheapestCostNow = ResourceAllocator::subsetCost(I, *pick);
            f.costPerSkillNow = (req > 0) ? (f.cheapestCostNow / (double)req) : f.cheapestCostNow;

            double sum = 0.0;
            double mn = std::numeric_limits<double>::infinity();

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
                f.minWageAvail = std::numeric_limits<double>::infinity();
                f.avgWageAvail = std::numeric_limits<double>::infinity();
            }
        }
        else {
            f.cheapestCostNow = std::numeric_limits<double>::infinity();
            f.costPerSkillNow = std::numeric_limits<double>::infinity();
            f.teamSizeMinNow = std::numeric_limits<double>::infinity();
            f.minWageAvail = std::numeric_limits<double>::infinity();
            f.avgWageAvail = std::numeric_limits<double>::infinity();
        }
    }


    const auto& S = gp::getFeatureScaling();

    auto norm01 = [](double val, double maxVal) -> double {
        if (!std::isfinite(val)) return 1.0;
        if (maxVal <= 0.0)     return 0.0;
        double x = val / maxVal;
        if (x < 0.0) x = 0.0;
        if (x > 1.0) x = 1.0;
        return x;
        };

    f.duration = norm01(f.duration, S.maxDuration);
    f.reqLevel = norm01(f.reqLevel, S.maxReqLevel);
    f.numTasks = norm01(f.numTasks, S.maxNumTasks);
    f.numResources = norm01(f.numResources, S.maxNumResources);
    f.numSkills = norm01(f.numSkills, S.maxNumSkills);
    f.taskResCount = norm01(f.taskResCount, S.maxTeamSizeMinNow);
    f.avgResCostForSkill = norm01(f.avgResCostForSkill, S.maxAvgResCostForSkill);
    f.unschedTasks = norm01(f.unschedTasks, S.maxUnschedTasks);
    f.availSkill = norm01(f.availSkill, S.maxAvailSkill);

    {
        double minGap = -S.maxReqLevel;
        double maxGap = S.maxAvailGapPos;
        if (!std::isfinite(f.availGap)) {
            f.availGap = 1.0;
        }
        else {
            double x = (f.availGap - minGap) / (maxGap - minGap);
            if (x < 0.0) x = 0.0;
            if (x > 1.0) x = 1.0;
            f.availGap = x;
        }
    }

    f.waitRes = norm01(f.waitRes, S.maxWaitRes);
    f.estPrec = norm01(f.estPrec, S.maxEstPrec);
    f.critLen = norm01(f.critLen, S.maxCritLen);
    f.slack = norm01(f.slack, S.maxSlackPos);
    f.succCount = norm01(f.succCount, S.maxSuccCount);
    f.totPred = norm01(f.totPred, S.maxTotPred);
    f.cheapestCostNow = norm01(f.cheapestCostNow, S.maxCheapestCostNow);
    f.costPerSkillNow = norm01(f.costPerSkillNow, S.maxCostPerSkillNow);
    f.teamSizeMinNow = norm01(f.teamSizeMinNow, S.maxTeamSizeMinNow);
    f.minWageAvail = norm01(f.minWageAvail, S.maxMinWageAvail);
    f.avgWageAvail = norm01(f.avgWageAvail, S.maxAvgWageAvail);

    return f;
}


Features computeResourceFeatures(const Instance& I, const Task& t, const Resource& r, int now) {
    Features f{};


    f.RES_WAGE = r.salary;


    {
        auto it = r.skills.find(t.reqSkill);
        f.RES_SKILL = (it != r.skills.end()) ? (double)it->second : 0.0;
    }

    f.RES_FREE = std::max(0.0, (double)(r.busyUntil - now));

    f.RES_MULTI = (double)r.skills.size();

    f.RES_UTIL = (r.busyUntil > now) ? 1.0 : 0.0;

    const auto& S = gp::getFeatureScaling();
    auto norm01 = [](double val, double maxVal) -> double {
        if (!std::isfinite(val)) return 1.0;
        if (maxVal <= 0.0) return 0.0;
        double x = val / maxVal;
        if (x < 0.0) x = 0.0;
        if (x > 1.0) x = 1.0;
        return x;
        };

    f.RES_WAGE = norm01(f.RES_WAGE, S.maxMinWageAvail);
    f.RES_SKILL = norm01(f.RES_SKILL, S.maxResSkillLevel);
    f.RES_FREE = norm01(f.RES_FREE, S.maxWaitRes);
    f.RES_MULTI = norm01(f.RES_MULTI, S.maxNumSkills);
    f.RES_UTIL = norm01(f.RES_UTIL, 1.0);

    f.resWage = f.RES_WAGE;
    f.resSkillLevel = f.RES_SKILL;
    f.resFreeTime = f.RES_FREE;
    f.resMultiSkill = f.RES_MULTI;
    f.resUtilization = f.RES_UTIL;

    return f;
}


namespace gp {

    static FeatureScaling g_scaling;
    static bool g_scalingReady = false;

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
        double sumSalary = 0.0;
        double maxResSkillLevelFound = 0.0;

        for (const auto& r : I.resources) {
            maxSalary = std::max(maxSalary, r.salary);
            sumSalary += r.salary;
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
        s.maxAvailSkill = std::max(1.0, maxResSkillLevelFound);
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
        g_scalingReady = true;
    }

    const FeatureScaling& getFeatureScaling() {
        return g_scaling;
    }

} // namespace gp
