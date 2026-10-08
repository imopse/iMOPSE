#pragma once

#include "BNTGPScheduleTypes.hpp"
#include "BNTGPSchedulingModel.hpp"

#include "../domain/Instance.hpp"

#include <vector>

namespace bntgp::decoding::msrcpsp::scheduling
{
    class BNTGPScheduler;

    class BNTGPSchedulingState final
    {
    public:
        BNTGPSchedulingState(
            domain::Instance& instance,
            const BNTGPSchedulingModel& model,
            const BNTGPScheduleOptions& options
        );

        BNTGPSchedulingState(
            const BNTGPSchedulingState&) = delete;

        BNTGPSchedulingState& operator=(
            const BNTGPSchedulingState&) = delete;

        [[nodiscard]]
        bool allTasksScheduled() const noexcept;

        [[nodiscard]]
        bool hasRunningTasks() const noexcept;

        [[nodiscard]]
        int nextRunningFinishTime() const noexcept;

        void freeResourcesAtCurrentTime() noexcept;

        void processFinishedAtCurrentTime();

        void advanceTo(int targetTime);

        [[nodiscard]]
        bool startTask(
            int taskIndex,
            int resourceId
        );

        [[nodiscard]]
        BNTGPScheduleResult releaseResult();

    private:
        friend class BNTGPScheduler;

        struct RunningTask final
        {
            int taskIndex{-1};
            int finishTime{0};
        };

        static void insertReadySorted(
            std::vector<int>& readyTasks,
            int taskIndex
        );

        static void eraseReadyValue(
            std::vector<int>& readyTasks,
            int taskIndex
        );

        domain::Instance& instance_;
        const BNTGPSchedulingModel& model_;
        BNTGPScheduleOptions options_{};

        std::vector<int> remainingPredecessors_{};
        std::vector<int> latestPredecessorFinish_{};
        std::vector<int> readyTasks_{};
        std::vector<unsigned char> readyFlags_{};
        std::vector<RunningTask> runningTasks_{};

        std::vector<double> reservePressureByResource_{};
        std::vector<double> criticalReserveByResource_{};
        std::vector<double> familyPressureByResourceFamily_{};

        int now_{0};
        int scheduledCount_{0};
        int unscheduledCount_{0};
        int makespan_{0};
        double totalCost_{0.0};

        std::vector<int> assignedResourceByImopseTask_{};
    };
}
