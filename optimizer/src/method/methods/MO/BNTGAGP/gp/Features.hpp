#pragma once
#include <string>
#include <vector>
#include <optional>
#include <limits>
#include <unordered_map>
#include "../domain/Instance.hpp"

struct PriorityContext {
    const Instance* inst = nullptr;
    int now = 0;
};

struct Features {
    double duration = 0.0;
    double reqLevel = 0.0;
    double availSkill = 0.0;

    bool feasibleNow = false;

    double critLen = 0.0;
    double slack = 0.0;
    double descCount = 0.0;
    double taskReleasePressure = 0.0;
    double taskCriticalPressure = 0.0;
    double availGap = 0.0;

    double cheapestCostNow = std::numeric_limits<double>::infinity();
    double costPerSkillNow = std::numeric_limits<double>::infinity();

    double minFeasibleCostNow = std::numeric_limits<double>::infinity();
    double costRegretNow = 0.0;

    double resWage = 0.0;
    double resSkill = 0.0;
    double resFree = 0.0;
    double resMulti = 0.0;
    double resUtil = 0.0;

    double resSkillLevel = 0.0;
    double resIdleTime = 0.0;
    double resCanStartNow = 0.0;
    double resUtilization = 0.0;
    double resWagePerLevel = 0.0;
    double resAssignCost = 0.0;
    double resAssignPremiumAll = 0.0;
    double resReservePressure = 0.0;
    double resFamilyMismatch = 0.0;
    double resFutureBranchFit = 0.0;
    double resBottleneckPreservation = 0.0;
    double resSpecialistMisuse = 0.0;
    double resRelativeWage = 0.0;

    double taskResCount = 0.0;
    double avgResCostForSkill = 0.0;
    double unschedTasks = 0.0;
};

struct SkillStepInfo {
    int maxFreeLevel = 0;
    std::vector<int> minWaitAtLeast;
    std::vector<double> cheapestAtLeast;
    std::vector<double> secondCheapestAtLeast;
};

void setFeaturePrecomputed(
    const std::vector<double>* taskResCountByTask,
    const std::vector<double>* avgResCostByTask,
    const std::unordered_map<int, int>* resIndexById,
    const int* unschedCountPtr,
    const std::vector<int>* remainingPredCountByTask,
    const std::vector<int>* latestPredFinishByTask,
    const std::unordered_map<std::string, SkillStepInfo>* skillStepCache,
    const std::vector<int>* matchedLevelByTaskRes,
    int matchedLevelResCount
);

void clearFeaturePrecomputed();
void setOptionalTaskFeatureUsage(
    bool needTaskStructureFeatures,
    bool needTaskResCount,
    bool needTaskAvgResCost,
    bool needTaskUnschedTasks,
    bool needTaskAvailabilityFeatures,
    bool needTaskCostNowFeatures,
    bool needTaskReleasePressure,
    bool needTaskCriticalPressure
);
void setOptionalResourceFeatureUsage(
    bool needFutureBranchFit,
    bool needBottleneckPreservation,
    bool needSpecialistMisuse
);
void buildPairEvalStepPrecomputed(
    const Instance& I,
    int now,
    const std::vector<int>& readyTaskIdx
);
void buildTaskEvalStepPrecomputed(
    const Instance& I,
    int now,
    const std::vector<int>& readyTaskIdx
);

void clearTaskEvalStepPrecomputed();
Features computeFeaturesFast(const PriorityContext& ctx, int taskIx);
void clearPairEvalStepPrecomputed();

Features computeFeatures(const PriorityContext& ctx, int taskIx);
Features computeResourceFeatures(const Instance& I, const Task& t, const Resource& r, int now);
Features computeResourceFeaturesFast(
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
);

Features computePairFeaturesFast(
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
);