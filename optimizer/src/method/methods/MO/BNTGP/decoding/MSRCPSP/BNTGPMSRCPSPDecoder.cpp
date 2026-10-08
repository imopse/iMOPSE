#include "BNTGPMSRCPSPDecoder.hpp"

#include "scheduling/FeatureScaling.hpp"

#include "problem/problems/MSRCPSP/CScheduler.h"

#include <cstddef>

namespace bntgp::decoding::msrcpsp
{
    BNTGPMSRCPSPDecoder::BNTGPMSRCPSPDecoder(
        const domain::Instance& sourceInstance,
        ::CScheduler& imopseScheduler,
        TreeParameters treeParameters)
        : workingInstance_(sourceInstance),
          imopseScheduler_(imopseScheduler),
          treeParameters_(treeParameters)
    {
        scaling_ = scheduling::buildFeatureScaling(
            workingInstance_
        );

        scheduling::buildCPM(
            workingInstance_,
            cpm_
        );

        scheduler_.emplace(
            workingInstance_,
            scaling_,
            cpm_
        );
    }

    domain::Instance&
        BNTGPMSRCPSPDecoder::resetWorkingInstance() noexcept
    {
        for (domain::Task& task : workingInstance_.tasks)
        {
            task.start = -1;
            task.finish = -1;
        }

        for (domain::Resource& resource : workingInstance_.resources)
        {
            resource.busy = false;
            resource.busyUntil = 0;
            resource.busyStart = 0;
            resource.totalBusy = 0;
        }

        return workingInstance_;
    }

    BNTGPEvaluation
        BNTGPMSRCPSPDecoder::decodeAndEvaluate(
            const gp::GPTree& tree)
    {
        domain::Instance& decodingInstance =
            resetWorkingInstance();

        const scheduling::BNTGPScheduleResult simulation =
            scheduler_->withPairTree(
                decodingInstance,
                tree
            );

        ::CScheduler& scheduler =
            imopseScheduler_;

        const std::size_t taskCount =
            scheduler.GetTasks().size();

        scheduler.Reset();

        for (std::size_t taskIndex = 0U;
            taskIndex < taskCount;
            ++taskIndex)
        {
            const ::TResourceID resourceId =
                static_cast<::TResourceID>(
                    simulation
                    .assignedResByImopseTaskIndex
                    .at(taskIndex)
                );

            scheduler.Assign(
                taskIndex,
                resourceId
            );
        }

        scheduler.BuildTimestamps_TA();

        BNTGPEvaluation evaluation{};

        evaluation.makespan =
            static_cast<int>(
                scheduler.EvaluateDuration()
            );

        evaluation.cost =
            static_cast<double>(
                scheduler.EvaluateCost()
            );

        const double minimumMakespan =
            static_cast<double>(
                scheduler.GetMinDuration()
            );

        const double maximumMakespan =
            static_cast<double>(
                scheduler.GetMaxDuration()
            );

        const double minimumCost =
            static_cast<double>(
                scheduler.GetMinCost()
            );

        const double maximumCost =
            static_cast<double>(
                scheduler.GetMaxCost()
            );

        evaluation.normalizedMakespan =
            maximumMakespan > minimumMakespan
            ? (
                static_cast<double>(evaluation.makespan)
                - minimumMakespan
              )
              /
              (
                maximumMakespan
                - minimumMakespan
              )
            : 0.0;

        evaluation.normalizedCost =
            maximumCost > minimumCost
            ? (
                evaluation.cost
                - minimumCost
              )
              /
              (
                maximumCost
                - minimumCost
              )
            : 0.0;

        evaluation.rawNormalizedMakespan = evaluation.normalizedMakespan;
        evaluation.rawNormalizedCost = evaluation.normalizedCost;
        evaluation.searchMakespan = static_cast<double>(evaluation.makespan);
        evaluation.searchCost = evaluation.cost;

        if (treeParameters_.softDepthEnabled && !tree.isEmpty()) {
            const std::size_t edgeDepth = tree.depth() - 1U;
            if (edgeDepth > static_cast<std::size_t>(treeParameters_.softDepthFreeEdges)) {
                const double excess = static_cast<double>(edgeDepth -
                    static_cast<std::size_t>(treeParameters_.softDepthFreeEdges));
                evaluation.searchMakespan += excess * treeParameters_.softDepthMakespanPenalty;
                evaluation.searchCost += excess * treeParameters_.softDepthCostPenalty;
                if (maximumMakespan > minimumMakespan)
                    evaluation.normalizedMakespan += excess * treeParameters_.softDepthMakespanPenalty /
                        (maximumMakespan - minimumMakespan);
                if (maximumCost > minimumCost)
                    evaluation.normalizedCost += excess * treeParameters_.softDepthCostPenalty /
                        (maximumCost - minimumCost);
            }
        }
        evaluation.fitness = evaluation.normalizedMakespan + evaluation.normalizedCost;

        return evaluation;
    }
}
