#include "CMSRCPSP_TA.h"
#include "../../../utils/logger/CExperimentLogger.h"

CMSRCPSP_TA::CMSRCPSP_TA(CScheduler& scheduler, size_t objCount)
    : m_Scheduler(scheduler)
    , m_ObjCount(objCount)
{
    CreateProblemEncoding();

    const std::vector<float> maxValues = {
            m_Scheduler.GetMaxDuration(),
            m_Scheduler.GetMaxCost(),
            m_Scheduler.GetMaxAvgCashFlowDev(),
            m_Scheduler.GetMaxSkillOveruse(),
            m_Scheduler.GetMaxAvgUseOfResTime()
    };
    m_MaxObjectiveValues = std::vector<float>(maxValues.begin(), maxValues.begin() + objCount);

    const std::vector<float> minValues = {
            m_Scheduler.GetMinDuration(),
            m_Scheduler.GetMinCost(),
            m_Scheduler.GetMinAvgCashFlowDev(),
            m_Scheduler.GetMinSkillOveruse(),
            m_Scheduler.GetMinAvgUseOfResTime()
    };
    m_MinObjectiveValues = std::vector<float>(minValues.begin(), minValues.begin() + objCount);
}

SProblemEncoding &CMSRCPSP_TA::GetProblemEncoding()
{
    return m_ProblemEncoding;
}

void CMSRCPSP_TA::Evaluate(AIndividual& individual)
{
    m_Scheduler.Reset();
    for (size_t i = 0; i < individual.m_Genotype.m_FloatGenotype.size(); ++i)
    {
        TResourceID selectedResourceId = m_CapableResources[i][(size_t)individual.m_Genotype.m_FloatGenotype[i]];
        m_Scheduler.Assign(i, selectedResourceId);
    }

    m_Scheduler.BuildTimestamps_TA();

    individual.m_Evaluation.resize(m_ObjCount);
    if (m_ObjCount > 0) { individual.m_Evaluation[0] = m_Scheduler.EvaluateDuration(); }
    if (m_ObjCount > 1) { individual.m_Evaluation[1] = m_Scheduler.EvaluateCost(); }
    if (m_ObjCount > 2) { individual.m_Evaluation[1] = m_Scheduler.EvaluateAvgCashFlowDev(); }
    if (m_ObjCount > 3) { individual.m_Evaluation[1] = m_Scheduler.EvaluateSkillOveruse(); }
    if (m_ObjCount > 4) { individual.m_Evaluation[1] = m_Scheduler.EvaluateAvgUseOfResTime(); }

    // Normalize
    for (int i = 0; i < individual.m_Evaluation.size(); ++i)
    {
        individual.m_NormalizedEvaluation[i] = (individual.m_Evaluation[i] - m_MinObjectiveValues[i]) / (m_MaxObjectiveValues[i] - m_MinObjectiveValues[i]);
    }
}

void CMSRCPSP_TA::LogSolution(AIndividual& individual)
{
    Evaluate(individual);
    CExperimentLogger::WriteSchedulerToFile(m_Scheduler, individual);
}

float CMSRCPSP_TA::FindBestGeneValueCostWise(size_t geneIdx) const
{
    float bestGeneValue = 0.f;
    float cheapestValue = FLT_MAX;
    const std::vector<TResourceID>& resourceIds = m_CapableResources[geneIdx];
    for (size_t i = 0; i < resourceIds.size(); ++i)
    {
        float salary = m_Scheduler.GetResourceById(resourceIds[i])->GetSalary();
        if (salary < cheapestValue)
        {
            cheapestValue = salary;
            bestGeneValue = (float)i;
        }
    }
    return bestGeneValue;
}

std::vector<size_t> CMSRCPSP_TA::FindNumberOfResourcesUse(const std::vector<float>& solution) const
{
    const std::vector<CResource>& resources = m_Scheduler.GetResources();
    std::vector<size_t> resourcesUsage(resources.size(), 0);

    for (size_t i = 0; i < solution.size(); ++i)
    {
        TResourceID selectedResourceId = m_CapableResources[i][(size_t)solution[i]];
        resourcesUsage[selectedResourceId - 1] += 1;
    }

    return resourcesUsage;
}

float CMSRCPSP_TA::FindBestGeneValueUsageWise(size_t geneIdx, const std::vector<size_t>& currentResourcesUsage) const
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

void CMSRCPSP_TA::CreateProblemEncoding()
{
    m_CapableResources.clear();
    m_UpperBounds.clear();

    const std::vector<CTask>& tasks = m_Scheduler.GetTasks();
    m_CapableResources.reserve(tasks.size());
    m_UpperBounds.reserve(tasks.size());

    for (const CTask& task: tasks)
    {
        std::vector<TResourceID> capableResourceIds;
        m_Scheduler.GetCapableResources(task, capableResourceIds);
        m_CapableResources.push_back(capableResourceIds);
        m_UpperBounds.push_back(capableResourceIds.size());
    }

    SEncodingSection associationSection;
    associationSection.m_SectionType = EEncodingType::ASSOCIATION;
    for (const size_t& ub: m_UpperBounds)
    {
        associationSection.m_SectionDescription.push_back({(float) 0, (float) ub});
    }
    m_ProblemEncoding = SProblemEncoding{(int)m_ObjCount, {associationSection}};
}
