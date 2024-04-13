#include "CScheduler.h"
#include <numeric>
#include <algorithm>
#include <limits>
#include <stack>

void CScheduler::SetCapableResources(const std::vector<std::vector<TResourceID>>& capableResources)
{
    m_CapableResources = capableResources;
}

void CScheduler::SetTasks(const std::vector<CTask> &tasks)
{
    m_Tasks = tasks;
}

void CScheduler::SetResources(const std::vector<CResource> &resources)
{
    m_Resources = resources;
}

const CTask* CScheduler::GetTaskById(TTaskID taskId) const
{
    return taskId <= m_Tasks.size() ? &m_Tasks[taskId - 1] : nullptr;
}

const CResource* CScheduler::GetResourceById(TResourceID resourceId) const
{
    return resourceId <= m_Resources.size() ? &m_Resources[resourceId - 1] : nullptr;
}

void CScheduler::Assign(size_t taskIndex, TResourceID resourceId)
{
    m_Tasks[taskIndex].SetResourceID(resourceId);
}

void CScheduler::BuildTimestamps_TA()
{
    for (CResource &resource: m_Resources)
    {
        resource.SetFinish(0);
        resource.SetWorkingTime(0);
    }

    // Reset tasks start
    for (size_t i = 0; i < m_Tasks.size(); ++i)
    {
        m_Tasks[i].SetStart(-1);
    }

    // Assign tasks with relation requirements
    for (size_t i = 0; i < m_Tasks.size(); ++i)
    {
        if (m_Tasks[i].GetHasSuccessors())
        {
            BuildTimestampForTask_TA(m_Tasks[i]);
        }
    }

    // Assign rest of the tasks
    for (size_t i = 0; i < m_Tasks.size(); ++i)
    {
        if (!m_Tasks[i].GetHasSuccessors())
        {
            BuildTimestampForTask_TA(m_Tasks[i]);
        }
    }
}

void CScheduler::BuildTimestamps_TO(std::vector<int>& tasksIndexes)
{
    for (CResource& resource : m_Resources)
    {
        resource.SetFinish(0);
        resource.SetWorkingTime(0);
    }

    for (size_t i = 0; i < m_Tasks.size(); ++i)
    {
        m_Tasks[i].SetStart(-1);
    }

    for (size_t i = 0; i < tasksIndexes.size(); ++i)
    {
        AssignTask(tasksIndexes[i]);
    }
}

void CScheduler::AssignTask(size_t taskIndex)
{
    if (m_Tasks[taskIndex].GetStart() == -1)
    {
        for (TTaskID predecessorId: m_Tasks[taskIndex].GetPredecessors())
        {
            if (m_Tasks[predecessorId - 1].GetResourceID() == -1)
            {
                AssignTask(predecessorId - 1);
            }
        }

        BuildTimestampForTask_TO(taskIndex);
    }
}

void CScheduler::BuildTimestampForTask_TA(CTask &task)
{
    CResource &resource = m_Resources[(size_t) task.GetResourceID() - 1];
    TTime start = std::max(GetEarliestTime(task), resource.GetFinish());
    task.SetStart(start);
    resource.SetFinish(start + task.GetDuration());
    resource.AddWorkingTime(task.GetDuration());
}

void CScheduler::BuildTimestampForTask_TO(size_t taskIndex)
{
    CTask& task = m_Tasks[taskIndex];
    TTime earliestTime = GetEarliestTime(task);

    TResourceID bestCapableResourceId = GetBestCapableResourceId(taskIndex, earliestTime);
    Assign(taskIndex, bestCapableResourceId);

    CResource& resource = m_Resources[task.GetResourceID() - 1];
    TTime start = std::max(earliestTime, resource.GetFinish());
    task.SetStart(start);
    resource.SetFinish(start + task.GetDuration());
    resource.AddWorkingTime(task.GetDuration());
}

TResourceID CScheduler::GetBestCapableResourceId(size_t taskIndex, TTime earliestTime)
{
    CTask& task = m_Tasks[taskIndex];

    TResourceID bestCapableResourceId = m_CapableResources[taskIndex][0];
    TTime earliestBestCapableResourceTime = std::numeric_limits<short>::max();

    for (int i = 0; i < m_CapableResources[taskIndex].size(); ++i)
    {
        CResource& resource = m_Resources[m_CapableResources[taskIndex][i] - 1];
        TTime startTime = std::max(resource.GetFinish(), earliestTime);

        if (startTime < earliestBestCapableResourceTime)
        {
            earliestBestCapableResourceTime = startTime;
            bestCapableResourceId = m_CapableResources[taskIndex][i];
        }
    }

    return bestCapableResourceId;
}

std::vector<size_t> CScheduler::GetTasksIndexes(std::vector<float>& priorities)
{
    std::vector<size_t> tasksIndexes(priorities.size());
    std::iota(tasksIndexes.begin(), tasksIndexes.end(), 0);

    std::stable_sort(tasksIndexes.begin(), tasksIndexes.end(),
        [&priorities](size_t i1, size_t i2) {return priorities[i1] > priorities[i2]; });

    return tasksIndexes;
}

void CScheduler::Clear()
{
    m_Tasks.clear();
    m_Resources.clear();
}

void CScheduler::Reset()
{
    for (CTask &task: m_Tasks)
    {
        task.SetStart(-1);
        task.SetResourceID(-1);
    }

    for (CResource &resource: m_Resources)
    {
        resource.SetFinish(-1);
    }
}

float CScheduler::EvaluateDuration()
{
    TTime duration = 0;
    for (const CResource &resource: m_Resources)
    {
        if (resource.GetFinish() > duration)
        {
            duration = resource.GetFinish();
        }
    }
    return duration;
}

float CScheduler::EvaluateCost()
{
    float cost = 0.f;
    for (const CTask &task: m_Tasks)
    {
        TResourceID resourceId = task.GetResourceID();
        if (resourceId > 0)
        {
            CResource &resource = m_Resources[(size_t) task.GetResourceID() - 1];
            cost += resource.GetSalary() * task.GetDuration();
        }
    }
    return cost;
}

float CScheduler::EvaluateAvgCashFlowDev()
{
    // Calculate total duration
    TTime duration = 0;
    for (const CResource &resource: m_Resources)
    {
        if (resource.GetFinish() > duration)
        {
            duration = resource.GetFinish();
        }
    }

    // Init time stamps with 0
    std::vector<float> cashFlows((size_t) duration, 0.f);
    float totalCashFlow = 0.f;
    for (const CTask &task: m_Tasks)
    {
        if (task.GetStart() >= 0)
        {
            TResourceID resourceId = task.GetResourceID();
            if (resourceId > 0)
            {
                CResource &resource = m_Resources[(size_t) task.GetResourceID() - 1];
                float resourceSalary = resource.GetSalary();
                for (TTime i = task.GetStart(); i < task.GetExpectedFinish(); ++i)
                {
                    cashFlows[i] += resourceSalary;
                    totalCashFlow += resourceSalary;
                }
            }
        }
    }

    float avgCashFlow = totalCashFlow / (float) duration;

    // Calculate deviation
    float totalCashFlowDeviation = 0.f;
    for (const float &cashFlow: cashFlows)
    {
        totalCashFlowDeviation += fabsf(cashFlow - avgCashFlow);
    }
    return totalCashFlowDeviation;
}

float CScheduler::EvaluateSkillOveruse()
{
    // In my opinion - it could be calculated differently
    // Now, it does not include resource's additional skills which are not required by a task
    float overuse = 0.f;
    for (const CTask &task: m_Tasks)
    {
        TResourceID resourceId = task.GetResourceID();
        if (resourceId > 0)
        {
            CResource &resource = m_Resources[(size_t) task.GetResourceID() - 1];
            for (const SSkill &reqSkill: task.GetRequiredSkills())
            {
                TSkillLevel resSkillLevel = 0;
                if (resource.GetSkillLevel(reqSkill.m_TypeID, resSkillLevel))
                {
                    overuse += (resSkillLevel - reqSkill.m_Level);
                }
            }
        }
    }
    return overuse;
}

float CScheduler::EvaluateAvgUseOfResTime()
{
    // It's more of a deviation from expected work time
    static const float s_ExpectedUsedTime = (float) GetMaxDuration() / (float) m_Resources.size();
    float workingTimeSum = 0.f;
    for (const CResource &res: m_Resources)
    {
        workingTimeSum += fabsf(res.GetWorkingTime() - s_ExpectedUsedTime);
    }
    return workingTimeSum;
}

void CScheduler::GetCapableResources(const CTask &task, std::vector<TResourceID> &resourceIds) const
{
    resourceIds.clear();
    for (const CResource &resource: m_Resources)
    {
        if (task.CanBeExecutedBy(resource))
        {
            resourceIds.push_back(resource.GetResourceID());
        }
    }
}

TTime CScheduler::GetEarliestTime(const CTask &task) const
{
    TTime earliestTime = 0;

    for (TTaskID predId: task.GetPredecessors())
    {
        TTime expectedFinish = m_Tasks[(size_t) predId - 1].GetExpectedFinish();
        if (expectedFinish > earliestTime)
        {
            earliestTime = expectedFinish;
        }
    }

    return earliestTime;
}

float CScheduler::GetMaxDuration() const
{
    TTime duration = 0;
    for (const CTask &task: m_Tasks)
    {
        duration += task.GetDuration();
    }
    return float(duration);
}

float CScheduler::GetMinDuration() const
{
    TTime minDuration = TIME_MAX;
    for (const CTask &task: m_Tasks)
    {
        const TTime &taskDuration = task.GetDuration();
        if (taskDuration < minDuration)
        {
            minDuration = taskDuration;
        }
    }

    return float((minDuration * (TTime) m_Tasks.size()) / (TTime) m_Resources.size());
}

float CScheduler::GetMaxCost() const
{
    float mostExpRes = 0.f;
    for (const CResource &resource: m_Resources)
    {
        mostExpRes = fmaxf(mostExpRes, resource.GetSalary());
    }
    float maxCost = 0.f;
    for (const CTask &task: m_Tasks)
    {
        maxCost += (task.GetDuration() * mostExpRes);
    }
    return maxCost;
}

float CScheduler::GetMinCost() const
{
    float minExpRes = FLT_MAX;
    for (const CResource &resource: m_Resources)
    {
        minExpRes = fminf(minExpRes, resource.GetSalary());
    }
    float minCost = 0.f;
    for (const CTask &task: m_Tasks)
    {
        minCost += (task.GetDuration() * minExpRes);
    }
    return minCost;
}

float CScheduler::GetMaxAvgCashFlowDev() const
{
    return GetMaxCost();
}

float CScheduler::GetMinAvgCashFlowDev() const
{
    return 0.f;
}

float CScheduler::GetMaxSkillOveruse() const
{
    float maxOveruse = 0.f;
    for (const CTask &task: m_Tasks)
    {
        for (const SSkill &reqSkill: task.GetRequiredSkills())
        {
            TSkillLevel maxSkillLevel = 0;
            for (const CResource &resource: m_Resources)
            {
                TSkillLevel resSkillLevel = 0;
                if (resource.GetSkillLevel(reqSkill.m_TypeID, resSkillLevel) && resSkillLevel > maxSkillLevel)
                {
                    maxSkillLevel = resSkillLevel;
                }
            }
            maxOveruse += (maxSkillLevel - reqSkill.m_Level);
        }
    }
    return maxOveruse;
}

float CScheduler::GetMinSkillOveruse() const
{
    return 0.f;
}

float CScheduler::GetMaxAvgUseOfResTime() const
{
    static const float s_ExpectedUsedTime = (float) GetMaxDuration() / (float) m_Resources.size();
    return s_ExpectedUsedTime * (m_Resources.size() - 1);
}

float CScheduler::GetMinAvgUseOfResTime() const
{
    return 0.f;
}

void CScheduler::SetInstanceName(const std::string &instanceName)
{
    m_InstanceName = instanceName;
}

std::string CScheduler::GetInstanceName() const
{
    return m_InstanceName;
}