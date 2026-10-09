#include "FeatureCalculationDetails.hpp"
#include "FeatureCalculationHelpers.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>

namespace bntgp::decoding::msrcpsp::scheduling::detail
{

Features computeResourceFeaturesFast(FeatureEvaluationState& state,
    const Instance& I,
    int taskIx,
    const Task& t,
    const Resource& r,
    int now,
    double cheapestNow,
    double cheapestCapableOverall,
    double waitOfCheapestCapableOverall,
    double reservePressureExcludingTask,
    double criticalReserveExcludingTask,
    double familyMismatchExcludingTask
) {
    (void)waitOfCheapestCapableOverall;

    Features f{};

    ResourceStepBase base{};
    if (tryGetResourceStepBaseCached(state, r, base)) {
        assignResourceStepBaseToFeatures(state, base, f);
    }
    else {
        base = computeResourceStepBaseRaw(state, r, now);
        assignResourceStepBaseToFeatures(state, base, f);
    }

    const bool canStartNowRaw = (f.resCanStartNow > 0.5);

    const int req = totalReq(state, t);
    const int lvlNow = matchedLevelCached(state, I, taskIx, t, r);

    f.resSkillLevel = (double)lvlNow;
    f.resWagePerLevel = r.salary / (double)std::max(1, lvlNow);
    f.resAssignCost = r.salary * (double)t.duration;

    f.resRelativeWage =
        std::isfinite(cheapestNow) ? (r.salary - cheapestNow) : 0.0;

    f.resAssignPremiumAll =
        std::isfinite(cheapestCapableOverall)
        ? std::max(0.0, r.salary - cheapestCapableOverall) * (double)t.duration
        : 0.0;

    f.resReservePressure = reservePressureExcludingTask;
    f.resFamilyMismatch = familyMismatchExcludingTask;

    if (state.needResourceFutureBranchFit) {
        f.resFutureBranchFit = futureBranchFitCached(state, I, taskIx, t, r);
    }

    if (state.needResourceBottleneckPreservation) {
        f.resBottleneckPreservation =
            bottleneckPreservationRaw(state, I, taskIx, t, criticalReserveExcludingTask);
    }

    if (state.needResourceSpecialistMisuse) {
        f.resSpecialistMisuse =
            specialistMisuseRaw(state, I, taskIx, t, r, criticalReserveExcludingTask);
    }

    const auto& S = *state.scaling;

    f.resWage = normalize(f.resWage, S.maxMinWageAvail);
    f.resSkillLevel = normalize(f.resSkillLevel, S.maxResSkillLevel);
    f.resIdleTime = normalize(f.resIdleTime, S.maxWaitRes);
    f.resCanStartNow = canStartNowRaw ? 1.0 : 0.0;
    f.resUtilization = normalize(f.resUtilization, 1.0);
    f.resWagePerLevel = normalize(f.resWagePerLevel, S.maxResWagePerLevel);
    f.resAssignCost = normalize(f.resAssignCost, S.maxMinWageAvail * S.maxDuration);
    f.resAssignPremiumAll = normalize(f.resAssignPremiumAll, S.maxMinWageAvail * S.maxDuration);
    f.resFamilyMismatch = normalize(f.resFamilyMismatch, S.maxResReservePressure);
    f.resFutureBranchFit = clamp01(f.resFutureBranchFit);
    f.resBottleneckPreservation = normalize(
        f.resBottleneckPreservation,
        S.maxResBottleneckPreservation
    );
    f.resSpecialistMisuse = normalize(
        f.resSpecialistMisuse,
        S.maxResSpecialistMisuse
    );
    f.resRelativeWage = normalize(f.resRelativeWage, S.maxResRelativeWage);
    f.resReservePressure = normalize(f.resReservePressure, S.maxResReservePressure);

    return f;
}

Features computePairFeaturesFast(FeatureEvaluationState& state,
    const Instance& I,
    int taskIx,
    const Task& t,
    const Resource& r,
    int now,
    double cheapestNow,
    double cheapestCapableOverall,
    double waitOfCheapestCapableOverall,
    double reservePressureExcludingTask,
    double criticalReserveExcludingTask,
    double familyMismatchExcludingTask
) {
    Features f{};

    if (state.pairEvaluationStepReady &&
        taskIx >= 0 &&
        taskIx < (int)state.pairBaseTaskFeatures.size() &&
        taskIx < (int)state.pairBaseTaskStamp.size() &&
        state.pairBaseTaskStamp[taskIx] == state.pairCurrentStamp) {
        f = state.pairBaseTaskFeatures[taskIx];
    }
    else {
        PriorityContext ctx;
        ctx.inst = &I;
        ctx.now = now;
        f = computeFeatures(state, ctx, taskIx);
    }

    Features rf = computeResourceFeaturesFast(state,
        I,
        taskIx,
        t,
        r,
        now,
        cheapestNow,
        cheapestCapableOverall,
        waitOfCheapestCapableOverall,
        reservePressureExcludingTask,
        criticalReserveExcludingTask,
        familyMismatchExcludingTask
    );

    f.resWage = rf.resWage;
    f.resSkillLevel = rf.resSkillLevel;
    f.resIdleTime = rf.resIdleTime;
    f.resCanStartNow = rf.resCanStartNow;
    f.resUtilization = rf.resUtilization;
    f.resWagePerLevel = rf.resWagePerLevel;
    f.resAssignCost = rf.resAssignCost;
    f.resAssignPremiumAll = rf.resAssignPremiumAll;
    f.resReservePressure = rf.resReservePressure;
    f.resFamilyMismatch = rf.resFamilyMismatch;
    f.resFutureBranchFit = rf.resFutureBranchFit;
    f.resBottleneckPreservation = rf.resBottleneckPreservation;
    f.resSpecialistMisuse = rf.resSpecialistMisuse;
    f.resRelativeWage = rf.resRelativeWage;

    return f;
}
}
