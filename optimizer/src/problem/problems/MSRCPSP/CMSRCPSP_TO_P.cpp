#include "CMSRCPSP_TO_P.h"
#include "../../../utils/logger/CExperimentLogger.h"

CMSRCPSP_TO_P::CMSRCPSP_TO_P(CScheduler& scheduler) : m_Scheduler(scheduler)
{
    CreateProblemEncoding();
    m_Scheduler.SetCapableResources(m_CapableResources);
   m_Scheduler.InitUnassignedTasks();

    m_MaxObjectiveValues = {
            m_Scheduler.GetMaxDuration(),
            m_Scheduler.GetMaxCost(),
            m_Scheduler.GetMaxAvgCashFlowDev(),
            m_Scheduler.GetMaxSkillOveruse(),
            m_Scheduler.GetMaxAvgUseOfResTime()
    };

    m_MinObjectiveValues = {
            m_Scheduler.GetMinDuration(),
            m_Scheduler.GetMinCost(),
            m_Scheduler.GetMinAvgCashFlowDev(),
            m_Scheduler.GetMinSkillOveruse(),
            m_Scheduler.GetMinAvgUseOfResTime()
    };
}

SProblemEncoding& CMSRCPSP_TO_P::GetProblemEncoding()
{
    return m_ProblemEncoding;
}

void CMSRCPSP_TO_P::Evaluate(AIndividual& individual)
{
    m_Scheduler.Reset();
    m_Scheduler.BuildTimestamps_TO_P(individual.m_Genotype.m_IntGenotype);

    // We assume this is 5 dim problem
    individual.m_Evaluation =
    {
            m_Scheduler.EvaluateDuration(),
            m_Scheduler.EvaluateCost(),
            m_Scheduler.EvaluateAvgCashFlowDev(),
            m_Scheduler.EvaluateSkillOveruse(),
            m_Scheduler.EvaluateAvgUseOfResTime()
    };

    // Normalize
    for (int i = 0; i < 5; i++)
    {
        individual.m_NormalizedEvaluation[i] = (individual.m_Evaluation[i] - m_MinObjectiveValues[i]) / (m_MaxObjectiveValues[i] - m_MinObjectiveValues[i]);
    }
}

void CMSRCPSP_TO_P::LogSolution(AIndividual& individual)
{
    Evaluate(individual);
    CExperimentLogger::WriteSchedulerToFile(m_Scheduler, individual);
}

void CMSRCPSP_TO_P::CreateProblemEncoding()
{
    m_CapableResources.clear();

    const std::vector<CTask>& tasks = m_Scheduler.GetTasks();
    m_CapableResources.reserve(tasks.size());

    for (const CTask& task : tasks)
    {
        std::vector<TResourceID> capableResourceIds;
        m_Scheduler.GetCapableResources(task, capableResourceIds);
        m_CapableResources.push_back(capableResourceIds);
    }

    size_t tasksSize = tasks.size();
    SEncodingSection permutationSection = SEncodingSection
    {
        std::vector<SEncodingDescriptor>(tasksSize, SEncodingDescriptor{
                (float)0, (float)(tasksSize - 1)
        }),
        EEncodingType::PERMUTATION
    };
    
    m_ProblemEncoding = SProblemEncoding{ 5, {permutationSection} };
}
