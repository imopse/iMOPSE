#pragma once

#include <string>
#include "CTask.h"

class CScheduler
{
public:
    const std::vector<std::vector<TResourceID>>& GetCapableResources() const
    { return m_CapableResources; };

    const std::vector<CTask> &GetTasks() const
    { return m_Tasks; };

    const std::vector<CResource> &GetResources() const
    { return m_Resources; };

    void SetInstanceName(const std::string& instanceName);
    void SetCapableResources(const std::vector<std::vector<TResourceID>>& capableResources);
    void SetTasks(const std::vector<CTask> &tasks);
    void SetResources(const std::vector<CResource> &resources);
    void InitUnassignedTasks();

    const CTask* GetTaskById(TTaskID taskId) const;
    const CResource* GetResourceById(TResourceID resourceId) const;

    void Assign(size_t taskIndex, TResourceID resourceId);
    void BuildTimestamps_TA();

    void BuildTimestamps_TO_P(std::vector<int>& tasksIndexes);
    void BuildTimestamps_TO_A(std::vector<float>& priorities);
    void CheckUnassignedTask(size_t taskIndex);

    void BuildTimestampForTask_TA(CTask &task);
    void BuildTimestampForTask_TO(size_t taskIndex);

    void Clear();
    void Reset();

    float EvaluateDuration();
    float EvaluateCost();
    float EvaluateAvgCashFlowDev();
    float EvaluateSkillOveruse();
    float EvaluateAvgUseOfResTime();
    void GetCapableResources(const CTask &task, std::vector<TResourceID> &resourceIds) const;
    TTime GetEarliestTime(const CTask &task) const;

    std::string GetInstanceName() const;
    float GetMaxDuration() const;
    float GetMinDuration() const;
    float GetMaxCost() const;
    float GetMinCost() const;
    float GetMaxAvgCashFlowDev() const;
    float GetMinAvgCashFlowDev() const;
    float GetMaxSkillOveruse() const;
    float GetMinSkillOveruse() const;
    float GetMaxAvgUseOfResTime() const;
    float GetMinAvgUseOfResTime() const;

private:
    std::string m_InstanceName;
    std::vector<std::vector<TResourceID>> m_CapableResources;
    std::vector<CTask> m_Tasks;
    std::vector<CResource> m_Resources;
    std::vector<std::vector<size_t>> m_UnassignedTasksIndexes;

    TTime GetBestCapableResourceId(size_t taskIndex, TTime earliestTime);
    std::vector<size_t> GetTasksIndexes(std::vector<float>& priorities);
};
