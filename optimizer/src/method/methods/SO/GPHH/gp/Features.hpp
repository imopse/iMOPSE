#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "../domain/Instance.hpp"

namespace gphh_so {

    struct PriorityContext {
        const Instance* inst = nullptr;
        int now = 0;
    };

    struct Features {
        bool feasibleNow = false;

        double taskUnlockBucket = 0.0;
        double taskScarcityBucket = 0.0;
        double taskLongFlag = 0.0;
        double taskReadyAge = 0.0;

        double resCostPremium = 0.0;
        double resScarceFamilyLoad = 0.0;
        double resWaitIfChosen = 0.0;
        double pairSpecialistMisuse = 0.0;
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

}                     