#include "BNTGPScheduler.hpp"

#include "BNTGPPairTreeScorer.hpp"
#include "BNTGPFeatureRequirements.hpp"
#include "BNTGPFeatureEvaluator.hpp"
#include "BNTGPSchedulingState.hpp"

#include "Features.hpp"
#include "ResourceAllocator.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

namespace bntgp::decoding::msrcpsp::scheduling
{
    namespace
    {
        using domain::Instance;
        using domain::Resource;
        using domain::Task;
        using scheduling::Features;
        using scheduling::ResourceAllocator;
        using scheduling::SkillStepInfo;

        struct TaskCostContext final
        {
            double cheapestNow{
                std::numeric_limits<double>::infinity()
            };
            double cheapestCapableOverall{
                std::numeric_limits<double>::infinity()
            };
            double waitOfCheapestCapableOverall{
                std::numeric_limits<double>::infinity()
            };
        };

        [[nodiscard]]
        TaskCostContext buildTaskCostContext(
            const Instance& instance,
            const std::vector<int>& candidateResourceIndices,
            const int now)
        {
            TaskCostContext context;

            for (const int resourceIndex : candidateResourceIndices)
            {
                const Resource& resource =
                    instance.resources[resourceIndex];
                const double wait = resource.busyUntil > now
                    ? static_cast<double>(resource.busyUntil - now)
                    : 0.0;

                if (resource.salary < context.cheapestCapableOverall)
                {
                    context.cheapestCapableOverall = resource.salary;
                    context.waitOfCheapestCapableOverall = wait;
                }
                else if (
                    resource.salary == context.cheapestCapableOverall &&
                    wait < context.waitOfCheapestCapableOverall)
                {
                    context.waitOfCheapestCapableOverall = wait;
                }

                if (resource.busyUntil <= now)
                {
                    context.cheapestNow = std::min(
                        context.cheapestNow,
                        resource.salary
                    );
                }
            }

            return context;
        }

        [[nodiscard]]
        int waitUntilAnyCapableResourceIsFree(
            const Instance& instance,
            const Task& task,
            const int now)
        {
            if (task.capableResourceIndices.empty())
            {
                return std::numeric_limits<int>::max() / 4;
            }

            int bestWait = std::numeric_limits<int>::max() / 4;

            for (const int resourceIndex :
                 task.capableResourceIndices)
            {
                if (resourceIndex < 0 ||
                    resourceIndex >=
                        static_cast<int>(instance.resources.size()))
                {
                    continue;
                }

                bestWait = std::min(
                    bestWait,
                    std::max(
                        0,
                        instance.resources[resourceIndex].busyUntil - now
                    )
                );
            }

            return bestWait;
        }

        class FamilyPressureSummary final
        {
        public:
            void rebuild(
                const std::vector<double>& familyPressure,
                const int resourceCount,
                const int familyCount)
            {
                bestPressureByResource_.assign(resourceCount, 0.0);
                secondBestPressureByResource_.assign(resourceCount, 0.0);
                bestFamilyByResource_.assign(resourceCount, -1);

                for (int resourceIndex = 0;
                     resourceIndex < resourceCount;
                     ++resourceIndex)
                {
                    const std::size_t base =
                        static_cast<std::size_t>(resourceIndex) *
                        static_cast<std::size_t>(familyCount);

                    double bestPressure = 0.0;
                    double secondBestPressure = 0.0;
                    int bestFamily = -1;

                    for (int family = 0;
                         family < familyCount;
                         ++family)
                    {
                        const double pressure = familyPressure[
                            base + static_cast<std::size_t>(family)
                        ];

                        if (pressure > bestPressure)
                        {
                            secondBestPressure = bestPressure;
                            bestPressure = pressure;
                            bestFamily = family;
                        }
                        else if (pressure > secondBestPressure)
                        {
                            secondBestPressure = pressure;
                        }
                    }

                    bestPressureByResource_[resourceIndex] =
                        bestPressure;
                    secondBestPressureByResource_[resourceIndex] =
                        secondBestPressure;
                    bestFamilyByResource_[resourceIndex] = bestFamily;
                }
            }

            [[nodiscard]]
            double mismatchExcludingTask(
                const std::vector<double>& familyPressure,
                const int familyCount,
                const int resourceIndex,
                const int taskFamily,
                const double taskReserveWeight) const noexcept
            {
                const std::size_t base =
                    static_cast<std::size_t>(resourceIndex) *
                    static_cast<std::size_t>(familyCount);

                double currentFamilyPressure = familyPressure[
                    base + static_cast<std::size_t>(taskFamily)
                ] - taskReserveWeight;

                if (currentFamilyPressure < 0.0)
                {
                    currentFamilyPressure = 0.0;
                }

                const double bestOtherFamilyPressure =
                    bestFamilyByResource_[resourceIndex] == taskFamily
                        ? secondBestPressureByResource_[resourceIndex]
                        : bestPressureByResource_[resourceIndex];

                return bestOtherFamilyPressure /
                    (1.0 + currentFamilyPressure);
            }

        private:
            std::vector<double> bestPressureByResource_{};
            std::vector<double> secondBestPressureByResource_{};
            std::vector<int> bestFamilyByResource_{};
        };
    }

    BNTGPScheduler::BNTGPScheduler(
        const domain::Instance& instance,
        const scheduling::FeatureScaling& scaling,
        const scheduling::CPMPrecalc& cpm)
        : scaling_(scaling),
          cpm_(cpm),
          model_(instance, scaling_, cpm_)
    {
    }

    BNTGPScheduleResult BNTGPScheduler::withPairTree(
        domain::Instance& instance,
        const gp::GPTree& pairTree,
        const BNTGPScheduleOptions& options) const
    {
        BNTGPPairTreeScorer pairScorer{pairTree};
        const BNTGPFeatureRequirements requirements{pairScorer};
        BNTGPFeatureEvaluator featureEvaluator{
            model_,
            scaling_,
            cpm_,
            requirements
        };
        const ResourceAllocator resourceAllocator{
            model_.resourceIdsBySkill(),
            model_.resourceIndexById(),
            model_.skillLevelsBySkill()
        };
        BNTGPSchedulingState state{
            instance,
            model_,
            options
        };

        std::unordered_map<std::string, SkillStepInfo> skillStepCache;

        if (requirements.needsTaskAvailabilityFeatures() ||
            requirements.needsTaskCostNowFeatures())
        {
            skillStepCache.reserve(
                model_.skillLevelsBySkill().size()
            );
        }

        FamilyPressureSummary familyPressureSummary;

        while (!state.allTasksScheduled())
        {
            state.freeResourcesAtCurrentTime();
            bool startedAnyTask = false;

            if (requirements.needsTaskAvailabilityFeatures() ||
                requirements.needsTaskCostNowFeatures())
            {
                model_.buildSkillStepCache(
                    instance,
                    state.now_,
                    skillStepCache
                );
            }
            else
            {
                skillStepCache.clear();
            }

            featureEvaluator.bindScheduleState(
                state.unscheduledCount_,
                state.remainingPredecessors_,
                state.latestPredecessorFinish_,
                skillStepCache
            );

            while (!state.readyTasks_.empty())
            {
                featureEvaluator.prepareDecisionStep(
                    instance,
                    state.now_,
                    state.readyTasks_
                );

                int bestTaskIndex = -1;
                int bestResourceId = -1;
                double bestScore =
                    std::numeric_limits<double>::infinity();
                int minimumFeasibleWait =
                    std::numeric_limits<int>::max() / 4;

                if (requirements.resourceFamilyMismatch &&
                    model_.familyCount() > 0)
                {
                    familyPressureSummary.rebuild(
                        state.familyPressureByResourceFamily_,
                        static_cast<int>(model_.resourceCount()),
                        model_.familyCount()
                    );
                }

                for (const int taskIndex : state.readyTasks_)
                {
                    Task& task = instance.tasks[taskIndex];
                    const std::vector<int>& candidateResources =
                        model_.scoringResourceIndices(taskIndex);
                    const TaskCostContext costContext =
                        buildTaskCostContext(
                            instance,
                            candidateResources,
                            state.now_
                        );

                    int localBestResourceId = -1;
                    double localBestScore =
                        std::numeric_limits<double>::infinity();

                    for (const int resourceIndex : candidateResources)
                    {
                        const Resource& resource =
                            instance.resources[resourceIndex];

                        double reservePressureExcludingTask = 0.0;

                        if (requirements.resourceReservePressure)
                        {
                            reservePressureExcludingTask =
                                state.reservePressureByResource_[resourceIndex] -
                                model_.reservePressureWeightByTask()[taskIndex];

                            if (reservePressureExcludingTask < 0.0)
                            {
                                reservePressureExcludingTask = 0.0;
                            }
                        }

                        double criticalReserveExcludingTask = 0.0;

                        if (requirements.resourceBottleneckPreservation ||
                            requirements.resourceSpecialistMisuse)
                        {
                            criticalReserveExcludingTask =
                                state.criticalReserveByResource_[resourceIndex] -
                                model_.criticalReserveWeightByTask()[taskIndex];

                            if (criticalReserveExcludingTask < 0.0)
                            {
                                criticalReserveExcludingTask = 0.0;
                            }
                        }

                        double familyMismatchExcludingTask = 0.0;

                        if (requirements.resourceFamilyMismatch &&
                            model_.familyCount() > 0)
                        {
                            familyMismatchExcludingTask =
                                familyPressureSummary.mismatchExcludingTask(
                                    state.familyPressureByResourceFamily_,
                                    model_.familyCount(),
                                    resourceIndex,
                                    model_.familyIdByTask()[taskIndex],
                                    model_.reservePressureWeightByTask()[taskIndex]
                                );
                        }

                        const Features features =
                            featureEvaluator.computePairFeatures(
                                instance,
                                taskIndex,
                                task,
                                resource,
                                state.now_,
                                costContext.cheapestNow,
                                costContext.cheapestCapableOverall,
                                costContext.waitOfCheapestCapableOverall,
                                reservePressureExcludingTask,
                                criticalReserveExcludingTask,
                                familyMismatchExcludingTask
                            );

                        const double score = pairScorer.score(features);

                        if (score < localBestScore)
                        {
                            localBestScore = score;
                            localBestResourceId = resource.id;
                        }
                    }

                    if (localBestResourceId < 0)
                    {
                        const int wait = !task.capableResources.empty()
                            ? waitUntilAnyCapableResourceIsFree(
                                  instance,
                                  task,
                                  state.now_
                              )
                            : resourceAllocator.waitUntilFeasible(
                                  instance,
                                  state.now_,
                                  task.reqSkill,
                                  task.reqLevel
                              );

                        minimumFeasibleWait = std::min(
                            minimumFeasibleWait,
                            wait
                        );
                        continue;
                    }

                    if (localBestScore < bestScore)
                    {
                        bestTaskIndex = taskIndex;
                        bestResourceId = localBestResourceId;
                        bestScore = localBestScore;
                    }
                }

                if (bestTaskIndex == -1)
                {
                    if (minimumFeasibleWait <= 0)
                    {
                        break;
                    }

                    if (minimumFeasibleWait <
                        std::numeric_limits<int>::max() / 8)
                    {
                        state.now_ += minimumFeasibleWait;
                        state.freeResourcesAtCurrentTime();
                        continue;
                    }

                    break;
                }

                const bool waited = state.startTask(
                    bestTaskIndex,
                    bestResourceId
                );
                startedAnyTask = true;

                if (waited)
                {
                    break;
                }
            }

            if (!startedAnyTask)
            {
                if (!state.hasRunningTasks())
                {
                    break;
                }

                state.advanceTo(
                    state.nextRunningFinishTime()
                );
            }
        }

        return state.releaseResult();
    }
}
