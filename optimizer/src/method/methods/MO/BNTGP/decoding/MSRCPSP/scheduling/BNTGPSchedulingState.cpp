#include "BNTGPSchedulingState.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace bntgp::decoding::msrcpsp::scheduling
{
    BNTGPSchedulingState::BNTGPSchedulingState(
        domain::Instance& instance,
        const BNTGPSchedulingModel& model,
        const BNTGPScheduleOptions& options)
        : instance_(instance),
          model_(model),
          options_(options),
          remainingPredecessors_(model.baseIndegrees()),
          latestPredecessorFinish_(model.taskCount(), 0),
          readyFlags_(model.taskCount(), 0),
          reservePressureByResource_(
              model.initialReservePressureByResource()
          ),
          criticalReserveByResource_(
              model.initialCriticalReserveByResource()
          ),
          familyPressureByResourceFamily_(
              model.initialFamilyPressureByResourceFamily()
          ),
          unscheduledCount_(static_cast<int>(model.taskCount()))
    {
        for (domain::Resource& resource : instance_.resources)
        {
            resource.busy = false;
            resource.busyUntil = 0;
            resource.busyStart = 0;
            resource.totalBusy = 0;
        }

        readyTasks_.reserve(model.taskCount());
        runningTasks_.reserve(model.taskCount());

        if (options_.captureAssignedResByImopse)
        {
            assignedResourceByImopseTask_.assign(
                model.taskCount(),
                -1
            );
        }

        for (int taskIndex = 0;
             taskIndex < static_cast<int>(model.taskCount());
             ++taskIndex)
        {
            if (remainingPredecessors_[taskIndex] == 0 &&
                instance_.tasks[taskIndex].start == -1)
            {
                readyTasks_.push_back(taskIndex);
                readyFlags_[taskIndex] = 1;
            }
        }
    }

    bool BNTGPSchedulingState::allTasksScheduled() const noexcept
    {
        return scheduledCount_ >= static_cast<int>(model_.taskCount());
    }

    bool BNTGPSchedulingState::hasRunningTasks() const noexcept
    {
        return !runningTasks_.empty();
    }

    int BNTGPSchedulingState::nextRunningFinishTime() const noexcept
    {
        int nextTime = std::numeric_limits<int>::max();

        for (const RunningTask& task : runningTasks_)
        {
            nextTime = std::min(nextTime, task.finishTime);
        }

        return nextTime;
    }

    void BNTGPSchedulingState::freeResourcesAtCurrentTime() noexcept
    {
        for (domain::Resource& resource : instance_.resources)
        {
            if (resource.busy && resource.busyUntil <= now_)
            {
                const int duration =
                    resource.busyUntil - resource.busyStart;

                if (duration > 0)
                {
                    resource.totalBusy += duration;
                }

                resource.busy = false;
            }
        }
    }

    void BNTGPSchedulingState::processFinishedAtCurrentTime()
    {
        std::size_t writeIndex = 0U;

        for (std::size_t index = 0U;
             index < runningTasks_.size();
             ++index)
        {
            const RunningTask& runningTask = runningTasks_[index];

            if (runningTask.finishTime == now_)
            {
                for (const int successorIndex :
                     model_.successors()[runningTask.taskIndex])
                {
                    if (instance_.tasks[successorIndex].start == -1)
                    {
                        --remainingPredecessors_[successorIndex];

                        if (now_ >
                            latestPredecessorFinish_[successorIndex])
                        {
                            latestPredecessorFinish_[successorIndex] = now_;
                        }

                        if (remainingPredecessors_[successorIndex] == 0 &&
                            readyFlags_[successorIndex] == 0)
                        {
                            insertReadySorted(
                                readyTasks_,
                                successorIndex
                            );
                            readyFlags_[successorIndex] = 1;
                        }
                    }
                }
            }
            else
            {
                if (writeIndex != index)
                {
                    runningTasks_[writeIndex] = runningTasks_[index];
                }

                ++writeIndex;
            }
        }

        runningTasks_.resize(writeIndex);
        freeResourcesAtCurrentTime();
    }

    void BNTGPSchedulingState::advanceTo(const int targetTime)
    {
        while (now_ < targetTime)
        {
            if (runningTasks_.empty())
            {
                now_ = targetTime;
                return;
            }

            const int nextTime = nextRunningFinishTime();

            if (nextTime > targetTime)
            {
                now_ = targetTime;
                return;
            }

            now_ = nextTime;
            processFinishedAtCurrentTime();
        }
    }

    bool BNTGPSchedulingState::startTask(
        const int taskIndex,
        const int resourceId)
    {
        domain::Task& task = instance_.tasks[taskIndex];

        int desiredStart = now_;
        const int resourceIndex = model_.resourceIndex(resourceId);

        if (resourceIndex >= 0)
        {
            desiredStart = std::max(
                desiredStart,
                instance_.resources[resourceIndex].busyUntil
            );
        }

        const bool waited = desiredStart > now_;

        if (waited)
        {
            advanceTo(desiredStart);
        }

        task.start = now_;
        task.finish = now_ + task.duration;

        if (options_.keepTaskAssignedResources)
        {
            task.assignedResources.clear();

            if (resourceId >= 0)
            {
                task.assignedResources.push_back(resourceId);
            }
        }

        if (options_.captureAssignedResByImopse &&
            resourceId >= 0 &&
            task.imopseIndex >= 0 &&
            task.imopseIndex <
                static_cast<int>(assignedResourceByImopseTask_.size()))
        {
            assignedResourceByImopseTask_[task.imopseIndex] = resourceId;
        }

        if (resourceIndex >= 0)
        {
            domain::Resource& resource =
                instance_.resources[resourceIndex];
            resource.busy = true;
            resource.busyStart = task.start;
            resource.busyUntil = task.finish;
        }

        if (options_.computeObjectiveStats && resourceIndex >= 0)
        {
            totalCost_ +=
                instance_.resources[resourceIndex].salary *
                static_cast<double>(task.duration);
        }

        runningTasks_.push_back({taskIndex, task.finish});

        if (options_.computeObjectiveStats)
        {
            makespan_ = std::max(makespan_, task.finish);
        }

        const double reserveWeight =
            model_.reservePressureWeightByTask()[taskIndex];
        const double criticalReserveWeight =
            model_.criticalReserveWeightByTask()[taskIndex];
        const int familyId = model_.familyIdByTask()[taskIndex];
        const int familyCount = model_.familyCount();

        for (const int candidateResourceIndex :
             model_.candidateResourceIndices(taskIndex))
        {
            if (reserveWeight > 0.0)
            {
                reservePressureByResource_[candidateResourceIndex] -=
                    reserveWeight;

                if (reservePressureByResource_[candidateResourceIndex] < 0.0)
                {
                    reservePressureByResource_[candidateResourceIndex] = 0.0;
                }

                if (familyCount > 0)
                {
                    const std::size_t pressureIndex =
                        static_cast<std::size_t>(candidateResourceIndex) *
                        static_cast<std::size_t>(familyCount) +
                        static_cast<std::size_t>(familyId);

                    familyPressureByResourceFamily_[pressureIndex] -=
                        reserveWeight;

                    if (familyPressureByResourceFamily_[pressureIndex] < 0.0)
                    {
                        familyPressureByResourceFamily_[pressureIndex] = 0.0;
                    }
                }
            }

            if (criticalReserveWeight > 0.0)
            {
                criticalReserveByResource_[candidateResourceIndex] -=
                    criticalReserveWeight;

                if (criticalReserveByResource_[candidateResourceIndex] < 0.0)
                {
                    criticalReserveByResource_[candidateResourceIndex] = 0.0;
                }
            }
        }

        eraseReadyValue(readyTasks_, taskIndex);
        readyFlags_[taskIndex] = 0;
        ++scheduledCount_;
        --unscheduledCount_;

        return waited;
    }

    BNTGPScheduleResult BNTGPSchedulingState::releaseResult()
    {
        BNTGPScheduleResult result;
        result.makespan = options_.computeObjectiveStats
            ? makespan_
            : 0;
        result.totalCost = options_.computeObjectiveStats
            ? totalCost_
            : 0.0;

        if (options_.captureAssignedResByImopse)
        {
            result.assignedResByImopseTaskIndex =
                std::move(assignedResourceByImopseTask_);
        }

        return result;
    }

    void BNTGPSchedulingState::insertReadySorted(
        std::vector<int>& readyTasks,
        const int taskIndex)
    {
        const auto insertionPoint = std::lower_bound(
            readyTasks.begin(),
            readyTasks.end(),
            taskIndex
        );

        if (insertionPoint == readyTasks.end() ||
            *insertionPoint != taskIndex)
        {
            readyTasks.insert(insertionPoint, taskIndex);
        }
    }

    void BNTGPSchedulingState::eraseReadyValue(
        std::vector<int>& readyTasks,
        const int taskIndex)
    {
        const auto taskIt = std::lower_bound(
            readyTasks.begin(),
            readyTasks.end(),
            taskIndex
        );

        if (taskIt != readyTasks.end() && *taskIt == taskIndex)
        {
            readyTasks.erase(taskIt);
        }
    }
}
