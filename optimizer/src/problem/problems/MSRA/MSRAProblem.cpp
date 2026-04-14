#include "MSRAProblem.h"
#include <algorithm>

CMSRAProblem::CMSRAProblem(CMSRAInstance& problemTemplate)
    : m_ProblemTemplate(problemTemplate)
{
    PrepareEncoding();

    m_MinObjectiveValues = { CalcMinEvalValue(0), CalcMinEvalValue(1) };
    m_MaxObjectiveValues = { CalcMaxEvalValue(0), CalcMaxEvalValue(1) };
}

SProblemEncoding& CMSRAProblem::GetProblemEncoding()
{
    return m_ProblemEncoding;
}

void CMSRAProblem::Evaluate(AIndividual& individual)
{
    //EvaluateWithoutFix(individual);
    EvaluateWithFix(individual);
}

void CMSRAProblem::LogSolution(AIndividual& individual)
{
    // TODO
}

float CMSRAProblem::GetUnassignGeneValue() const
{
    return (float)m_ProblemTemplate.GetTasks().size();
}

float CMSRAProblem::FindBestGeneValueByClosestTask(const std::vector<float>& solution, const size_t geneIdx) const
{
    size_t stageCount = m_ProblemTemplate.GetStageCount();
    size_t taskCount = m_ProblemTemplate.GetTasks().size();
    size_t resourceIdx = geneIdx / stageCount;
    size_t resourceFirstGeneIdx = resourceIdx * stageCount;
    size_t prevTask = (geneIdx > resourceFirstGeneIdx) ? (size_t)solution[geneIdx - 1] : taskCount;

    const auto& resourceTaskTransition = m_ProblemTemplate.GetResourceTaskTransition();
    const auto& taskTransition = resourceTaskTransition[resourceIdx];

    int minTransitionTime = INT_MAX;
    size_t closestTask = taskCount;
    for (size_t t = 0; t < taskCount; ++t)
    {
        int transitionTime = taskTransition[prevTask][t];
        if (transitionTime < minTransitionTime)
        {
            minTransitionTime = transitionTime;
            closestTask = t;
        }
    }
    return (float)closestTask;
}

float CMSRAProblem::FindBestGeneValueByProb(const size_t geneIdx) const
{
    size_t stageCount = m_ProblemTemplate.GetStageCount();
    size_t taskCount = m_ProblemTemplate.GetTasks().size();
    size_t resourceIdx = geneIdx / stageCount;

    const auto& probMtx = m_ProblemTemplate.GetProbMtx();
    const auto& resourceSuccProbMtx = probMtx[resourceIdx];

    float maxSuccProb = 0.f;
    size_t bestTask = taskCount;
    for (size_t t = 0; t < taskCount; ++t)
    {
        float succProb = resourceSuccProbMtx[t];
        if (succProb > maxSuccProb)
        {
            maxSuccProb = succProb;
            bestTask = t;
        }
    }
    return (float)bestTask;
}

void CMSRAProblem::EvaluateWithoutFix(AIndividual& individual)
{
    const auto& resources = m_ProblemTemplate.GetResources();
    const auto& tasks = m_ProblemTemplate.GetTasks();
    const auto& probMtx = m_ProblemTemplate.GetProbMtx();
    size_t resourceCount = resources.size();
    size_t taskCount = tasks.size();
    std::vector<int> usedAvailability(resourceCount, 0);
    std::vector<float> taskPersistProb(taskCount, 1.f);
    size_t stageCount = m_ProblemTemplate.GetStageCount();

    for (size_t r = 0; r < resourceCount; ++r)
    {
        size_t resourceGenes = (r * stageCount);
        size_t lastResourceTask = taskCount;
        int lastResourceWorkStage = -1;
        for (size_t s = 0; s < stageCount; ++s)
        {
            size_t taskIdx = (size_t)individual.m_Genotype.m_FloatGenotype[resourceGenes + s];
            if (CanResourceWorkOnTask(r, taskIdx, s, taskCount, lastResourceTask, lastResourceWorkStage))
            {
                if (usedAvailability[r] < resources[r].m_AvailableAmount)
                {
                    usedAvailability[r] += 1;
                    // TODO - experiment with different stacking of resource assigned to the same task at the same stage - e.g. take best or average
                    taskPersistProb[taskIdx] *= (1.f - probMtx[r][taskIdx]);

                    lastResourceWorkStage = (int)s;
                    lastResourceTask = taskIdx;
                }
                else
                {
                    // resource not used at this stage
                }
            }
        }
    }

    float totalThreat = 0.f;
    for (size_t t = 0; t < taskPersistProb.size(); ++t)
    {
        totalThreat += (taskPersistProb[t] * tasks[t].m_ThreatValue);
    }
    float totalCost = 0.f;
    for (size_t r = 0; r < resourceCount; ++r)
    {
        totalCost += float(usedAvailability[r] * resources[r].m_UnitCost);
    }
    individual.m_Evaluation = { totalThreat, totalCost };

    // Normalize
    for (int i = 0; i < individual.m_Evaluation.size(); ++i)
    {
        individual.m_NormalizedEvaluation[i] = (individual.m_Evaluation[i] - m_MinObjectiveValues[i]) / (m_MaxObjectiveValues[i] - m_MinObjectiveValues[i]);
    }
}

void CMSRAProblem::EvaluateWithFix(AIndividual& individual)
{
    const auto& resources = m_ProblemTemplate.GetResources();
    const auto& tasks = m_ProblemTemplate.GetTasks();
    const auto& probMtx = m_ProblemTemplate.GetProbMtx();
    size_t resourceCount = resources.size();
    size_t taskCount = tasks.size();
    std::vector<int> usedAvailability(resourceCount, 0);
    std::vector<float> taskPersistProb(taskCount, 1.f);
    size_t stageCount = m_ProblemTemplate.GetStageCount();

    for (size_t r = 0; r < resourceCount; ++r)
    {
        size_t resourceGenes = (r * stageCount);
        size_t lastResourceTask = taskCount;
        int lastResourceWorkStage = -1;
        for (size_t s = 0; s < stageCount; ++s)
        {
            size_t taskIdx = (size_t)individual.m_Genotype.m_FloatGenotype[resourceGenes + s];
            if (CanResourceWorkOnTask(r, taskIdx, s, taskCount, lastResourceTask, lastResourceWorkStage))
            {
                if (usedAvailability[r] < resources[r].m_AvailableAmount)
                {
                    usedAvailability[r] += 1;
                    // TODO - experiment with different stacking of weapons targeting the same target at the same stage - e.g. take best or average
                    taskPersistProb[taskIdx] *= (1.f - probMtx[r][taskIdx]);

                    lastResourceWorkStage = (int)s;
                    lastResourceTask = taskIdx;
                }
                else
                {
                    // resource not used at this stage
                }
            }
            else
            {
                individual.m_Genotype.m_FloatGenotype[resourceGenes + s] = taskCount;
            }
        }
    }

    float totalThreat = 0.f;
    for (size_t t = 0; t < taskPersistProb.size(); ++t)
    {
        totalThreat += (taskPersistProb[t] * tasks[t].m_ThreatValue);
    }
    float totalCost = 0.f;
    for (size_t r = 0; r < resourceCount; ++r)
    {
        totalCost += float(usedAvailability[r] * resources[r].m_UnitCost);
    }
    individual.m_Evaluation = { totalThreat, totalCost };

    // Normalize
    for (int i = 0; i < individual.m_Evaluation.size(); ++i)
    {
        individual.m_NormalizedEvaluation[i] = (individual.m_Evaluation[i] - m_MinObjectiveValues[i]) / (m_MaxObjectiveValues[i] - m_MinObjectiveValues[i]);
    }
}

float CMSRAProblem::CalcMinEvalValue(size_t objIdx) const
{
	switch (objIdx)
	{
	case 0:
	{
		return 0.f;
	}
	case 1:
	{
		return 0.f;
	}
	default:
		return 0.f;
	}
}

float CMSRAProblem::CalcMaxEvalValue(size_t objIdx) const
{
	switch (objIdx)
	{
	case 0:
	{
		int totalThreat = 0;
		for (const auto& t : m_ProblemTemplate.GetTasks())
		{
			totalThreat += t.m_ThreatValue;
		}
		return float(totalThreat);
	}
	case 1:
	{
		int maxSupplyUsed = 0;
		int stageCount = (int)m_ProblemTemplate.GetStageCount();
		for (const auto& r : m_ProblemTemplate.GetResources())
		{
            maxSupplyUsed += (std::min(r.m_AvailableAmount, stageCount) * r.m_UnitCost);
		}
		return float(maxSupplyUsed);
	}
	default:
		return 0.f;
	}
}

void CMSRAProblem::PrepareEncoding()
{
	size_t resourceCount = m_ProblemTemplate.GetResources().size();
	size_t stageCount = m_ProblemTemplate.GetStageCount();
	size_t taskCount = m_ProblemTemplate.GetTasks().size();
	size_t genotypeSize = resourceCount * stageCount;

	SEncodingSection associationSection;
	associationSection.m_SectionType = EEncodingType::ASSOCIATION;
	for (size_t i = 0; i < genotypeSize; ++i)
	{
		// Upper bound +1 as there is an extra option to do nothing
		associationSection.m_SectionDescription.push_back({ (float)0, (float)(taskCount + 1) });
	}
	m_ProblemEncoding = SProblemEncoding{2, {associationSection}};
}

bool CMSRAProblem::CanResourceWorkOnTask(size_t resourceIdx, size_t taskIdx, size_t stage, size_t taskCount, size_t lastResourceTask, int lastResourceWorkStage) const
{
	if (taskIdx < taskCount)
	{
		const auto& resourceTaskTransition = m_ProblemTemplate.GetResourceTaskTransition();
		const auto& taskTransition = resourceTaskTransition[resourceIdx];
		// TODO - transition time can be handled in different ways - at the moment: ignore blocked genes but do not fix anything
		int transitionTime = taskTransition[lastResourceTask][taskIdx];
		if (lastResourceWorkStage + transitionTime < (int)stage)
		{
			return true;
		}
	}

	return false;
}
