#include "CTS.h"
#include "../../../../utils/logger/CExperimentLogger.h"
#include "../utils/aggregatedFitness/CAggregatedFitness.h"
#include "../utils/experiment/CSOExperimentUtils.h"
#include "../../../../utils/logger/ErrorUtils.h"

CTS::CTS(std::vector<float>& objectiveWeights, AProblem& evaluator, AInitialization& initialization,
         SConfigMap* configMap)
        : ASOMethod(evaluator, initialization, objectiveWeights)
{
    configMap->TakeValue("TabuListSize", m_TabuListSize);
    ErrorUtils::LowerThanZeroI("TS", "TabuListSize", m_TabuListSize);

    configMap->TakeValue("MaxIterations", m_MaxIterations);
    ErrorUtils::LowerThanZeroI("TS", "MaxIterations", m_MaxIterations);

    configMap->TakeValue("SimilarityThreshold", m_SimilarityThreshold);
    ErrorUtils::LowerThanZeroF("TS", "SimilarityThreshold", m_SimilarityThreshold);
}


void CTS::RunOptimization()
{
    InitializeSolution();
    auto bestSolution = std::make_shared<SSOIndividual>(*m_CurrentSolution);
    CAggregatedFitness::CountFitness(*bestSolution, m_ObjectiveWeights);

    for (int iteration = 0; iteration < m_MaxIterations; ++iteration)
    {
        auto newSolution = std::shared_ptr<SSOIndividual>(m_Initialization.CreateNeighborSolution(m_Problem.GetProblemEncoding(), *m_CurrentSolution));
        m_Problem.Evaluate(*newSolution);
        CAggregatedFitness::CountFitness(*newSolution, m_ObjectiveWeights);

        double delta = CAggregatedFitness::CalculateDelta(*newSolution, *m_CurrentSolution, m_ObjectiveWeights);

        if (!IsTabu(newSolution) && delta < 0)
        {
            UpdateTabuList(newSolution);
            m_CurrentSolution = newSolution;

            if (delta < CAggregatedFitness::CalculateDelta(*bestSolution, *m_CurrentSolution, m_ObjectiveWeights))
            {
                bestSolution = std::make_shared<SSOIndividual>(*m_CurrentSolution);
                CAggregatedFitness::CountFitness(*bestSolution, m_ObjectiveWeights);
            }
        }
        
        CExperimentLogger::AddLine((std::to_string(delta) + ";" + std::to_string(m_CurrentSolution->m_Fitness)).c_str());
    }

    CSOExperimentUtils::LogResultData(*m_CurrentSolution, m_Problem);
}

void CTS::InitializeSolution()
{
    SProblemEncoding& problemEncoding = m_Problem.GetProblemEncoding();
    m_CurrentSolution = std::make_shared<SSOIndividual>(*m_Initialization.CreateSOIndividual(problemEncoding));

    m_Problem.Evaluate(*m_CurrentSolution);
    CAggregatedFitness::CountFitness(*m_CurrentSolution, m_ObjectiveWeights);
}

bool CTS::IsTabu(const std::shared_ptr<SSOIndividual> candidate)
{
    for (auto tabuSolution: m_TabuList)
    {
        // Check similarity for m_FloatGenotype
        int floatSimilarityCount = 0;
        for (size_t i = 0; i < candidate->m_Genotype.m_FloatGenotype.size(); i++)
        {
            if (std::fabs(candidate->m_Genotype.m_FloatGenotype[i] - tabuSolution->m_Genotype.m_FloatGenotype[i]) < 0.01)
            {
                floatSimilarityCount++;
            }
        }
        float floatSimilarityPercentage = static_cast<float>(floatSimilarityCount) / candidate->m_Genotype.m_FloatGenotype.size();
        if (floatSimilarityPercentage > m_SimilarityThreshold) return true;

        // Check similarity for m_IntGenotype
        int intSimilarityCount = 0;
        for (size_t i = 0; i < candidate->m_Genotype.m_IntGenotype.size(); i++)
        {
            if (candidate->m_Genotype.m_IntGenotype[i] == tabuSolution->m_Genotype.m_IntGenotype[i])
            {
                intSimilarityCount++;
            }
        }
        float intSimilarityPercentage = static_cast<float>(intSimilarityCount) / candidate->m_Genotype.m_IntGenotype.size();
        if (intSimilarityPercentage > m_SimilarityThreshold) return true;

        // Check similarity for m_BoolGenotype
        int boolSimilarityCount = 0;
        for (size_t i = 0; i < candidate->m_Genotype.m_BoolGenotype.size(); i++)
        {
            if (candidate->m_Genotype.m_BoolGenotype[i] == tabuSolution->m_Genotype.m_BoolGenotype[i])
            {
                boolSimilarityCount++;
            }
        }
        float boolSimilarityPercentage = static_cast<float>(boolSimilarityCount) / candidate->m_Genotype.m_BoolGenotype.size();
        if (boolSimilarityPercentage > m_SimilarityThreshold) return true;
    }

    return false; // Candidate is not tabu
}

void CTS::UpdateTabuList(std::shared_ptr<SSOIndividual> newSolution)
{
    m_TabuList.push_back(newSolution);
    
    if (m_TabuList.size() > m_TabuListSize)
    {
        m_TabuList.pop_front();
    }
}

