#include "CMSRCPSP_TO_FLOAT.h"
#include "../../../utils/logger/CExperimentLogger.h"
#include <numeric>
#include <algorithm>

CMSRCPSP_TO_FLOAT::CMSRCPSP_TO_FLOAT(CScheduler* scheduler, size_t objCount)
        : m_Scheduler(scheduler)
        , m_ObjCount(objCount)
{
    CreateProblemEncoding();
    m_Scheduler->SetCapableResources(m_CapableResources);

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

SProblemEncoding& CMSRCPSP_TO_FLOAT::GetProblemEncoding()
{
    return m_ProblemEncoding;
}

void CMSRCPSP_TO_FLOAT::Evaluate(AIndividual& individual)
{
    const std::vector<float>& floatGenotype = individual.m_Genotype.m_FloatGenotype;

    std::vector<int> taskIndexOrder(floatGenotype.size());
    std::iota(taskIndexOrder.begin(), taskIndexOrder.end(), 0);

    std::sort(taskIndexOrder.begin(), taskIndexOrder.end(),
              [&floatGenotype](int i, int j) {
                  return floatGenotype[i] < floatGenotype[j];
              });

    m_Scheduler->Reset();
    m_Scheduler->BuildTimestamps_TO(taskIndexOrder);
    
    individual.m_Evaluation =
    {
            m_Scheduler->EvaluateDuration(),
            m_Scheduler->EvaluateCost(),
            m_Scheduler->EvaluateAvgCashFlowDev(),
            m_Scheduler->EvaluateSkillOveruse(),
            m_Scheduler->EvaluateAvgUseOfResTime()
    };
    
    for (int i = 0; i < m_ObjCount; i++)
    {
        individual.m_NormalizedEvaluation[i] = (individual.m_Evaluation[i] - m_MinObjectiveValues[i]) / (m_MaxObjectiveValues[i] - m_MinObjectiveValues[i]);
    }
}

void CMSRCPSP_TO_FLOAT::LogSolution(AIndividual& individual)
{
    Evaluate(individual);
    CExperimentLogger::WriteSchedulerToFile(m_Scheduler, individual);
}

void CMSRCPSP_TO_FLOAT::CreateProblemEncoding()
{
    m_CapableResources.clear();

    const std::vector<CTask>& tasks = m_Scheduler->GetTasks();
    m_CapableResources.reserve(tasks.size());

    for (const CTask& task : tasks)
    {
        std::vector<TResourceID> capableResourceIds;
        m_Scheduler->GetCapableResources(task, capableResourceIds);
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
    
    m_ProblemEncoding = SProblemEncoding{ (int)m_ObjCount, {permutationSection} };
}
