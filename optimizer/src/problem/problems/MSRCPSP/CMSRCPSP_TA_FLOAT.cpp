#include "CMSRCPSP_TA_FLOAT.h"
#include "../../../utils/logger/CExperimentLogger.h"

CMSRCPSP_TA_FLOAT::CMSRCPSP_TA_FLOAT(CScheduler* scheduler, size_t objCount)
    : m_Scheduler(scheduler)
    , m_ObjCount(objCount)
{
    CreateProblemEncoding();

    m_MaxObjectiveValues = {
            m_Scheduler->GetMaxDuration(),
            m_Scheduler->GetMaxCost(),
            m_Scheduler->GetMaxAvgCashFlowDev(),
            m_Scheduler->GetMaxSkillOveruse(),
            m_Scheduler->GetMaxAvgUseOfResTime()
    };

    m_MinObjectiveValues = {
            m_Scheduler->GetMinDuration(),
            m_Scheduler->GetMinCost(),
            m_Scheduler->GetMinAvgCashFlowDev(),
            m_Scheduler->GetMinSkillOveruse(),
            m_Scheduler->GetMinAvgUseOfResTime()
    };
}

SProblemEncoding &CMSRCPSP_TA_FLOAT::GetProblemEncoding()
{
    return m_ProblemEncoding;
}

void CMSRCPSP_TA_FLOAT::Evaluate(AIndividual& individual)
{
    m_Scheduler->Reset();
    for (size_t i = 0; i < individual.m_Genotype.m_FloatGenotype.size(); ++i)
    {
        TResourceID selectedResourceId = m_CapableResources[i][(int)(individual.m_Genotype.m_FloatGenotype[i] * (float)(m_CapableResources[i].size() - 1))];
        m_Scheduler->Assign(i, selectedResourceId);
    }

    m_Scheduler->BuildTimestamps_TA();

    individual.m_Evaluation =
            {
                    m_Scheduler->EvaluateDuration(),
                    m_Scheduler->EvaluateCost(),
                    m_Scheduler->EvaluateAvgCashFlowDev(),
                    m_Scheduler->EvaluateSkillOveruse(),
                    m_Scheduler->EvaluateAvgUseOfResTime()
            };

    // Normalize
    for (int i = 0; i < m_ObjCount; i++)
    {
        individual.m_NormalizedEvaluation[i] = (individual.m_Evaluation[i] - m_MinObjectiveValues[i]) / (m_MaxObjectiveValues[i] - m_MinObjectiveValues[i]);
    }
}

void CMSRCPSP_TA_FLOAT::LogSolution(AIndividual& individual)
{
    Evaluate(individual);
    CExperimentLogger::WriteSchedulerToFile(m_Scheduler, individual);
}

float CMSRCPSP_TA_FLOAT::FindBestGeneValueCostWise(size_t geneIdx) const
{
    float bestGeneValue = 0.f;
    float cheapestValue = FLT_MAX;
    const std::vector<TResourceID>& resourceIds = m_CapableResources[geneIdx];
    for (size_t i = 0; i < resourceIds.size(); ++i)
    {
        float salary = m_Scheduler->GetResourceById(resourceIds[i])->GetSalary();
        if (salary < cheapestValue)
        {
            cheapestValue = salary;
            bestGeneValue = (float)i;
        }
    }
    return bestGeneValue;
}

std::vector<size_t> CMSRCPSP_TA_FLOAT::FindNumberOfResourcesUse(const std::vector<float>& solution) const
{
    const std::vector<CResource>& resources = m_Scheduler->GetResources();
    std::vector<size_t> resourcesUsage(resources.size(), 0);

    for (size_t i = 0; i < solution.size(); ++i)
    {
        TResourceID selectedResourceId = m_CapableResources[i][(size_t)solution[i]];
        resourcesUsage[selectedResourceId - 1] += 1;
    }

    return resourcesUsage;
}

float CMSRCPSP_TA_FLOAT::FindBestGeneValueUsageWise(size_t geneIdx, const std::vector<size_t>& currentResourcesUsage) const
{
    float bestGeneValue = 0.f;
    size_t smallestUsage = SIZE_MAX;
    const std::vector<TResourceID>& resourceIds = m_CapableResources[geneIdx];
    for (size_t i = 0; i < resourceIds.size(); ++i)
    {
        size_t usage = currentResourcesUsage[resourceIds[i] - 1];
        if (usage < smallestUsage)
        {
            smallestUsage = usage;
            bestGeneValue = (float)i;
        }
    }
    return bestGeneValue;
}

void CMSRCPSP_TA_FLOAT::CreateProblemEncoding()
{
    m_CapableResources.clear();
    m_UpperBounds.clear();

    const std::vector<CTask>& tasks = m_Scheduler->GetTasks();
    m_CapableResources.reserve(tasks.size());

    for (const CTask& task: tasks)
    {
        std::vector<TResourceID> capableResourceIds;
        m_Scheduler->GetCapableResources(task, capableResourceIds);
        m_CapableResources.push_back(capableResourceIds);
    }

    SEncodingSection associationSection;
    associationSection.m_SectionType = EEncodingType::FLOAT;
    for (int i = 0; i < tasks.size(); i++)
    {
        associationSection.m_SectionDescription.push_back({(float) 0, (float) 1});
    }
    m_ProblemEncoding = SProblemEncoding{(int)m_ObjCount, {associationSection}};
}
