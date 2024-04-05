#include "CSA.h"
#include "../../../../utils/logger/CExperimentLogger.h"
#include "../utils/aggregatedFitness/CAggregatedFitness.h"
#include "../../../../utils/random/CRandom.h"
#include "../utils/experiment/CSOExperimentUtils.h"
#include "../../../../utils/logger/ErrorUtils.h"
#include <cmath>

CSA::CSA(std::vector<float>& objectiveWeights, AProblem& evaluator, AInitialization& initialization,
         SConfigMap* configMap)
        : ASOMethod(evaluator, initialization, objectiveWeights)
{
    configMap->TakeValue("InitialTemperature", m_InitialTemperature);
    ErrorUtils::LowerThanZeroF("SA", "InitialTemperature", m_InitialTemperature);

    configMap->TakeValue("FinalTemperature", m_FinalTemperature);
    ErrorUtils::LowerThanZeroF("SA", "FinalTemperature", m_FinalTemperature);

    configMap->TakeValue("CoolingRate", m_CoolingRate);
    ErrorUtils::OutOfScopeF("SA", "CoolingRate", m_CoolingRate);
}


void CSA::RunOptimization()
{
    double temperature = m_InitialTemperature;
    
    InitializeSolution();

    while (temperature > m_FinalTemperature)
    {
        Iterate(temperature);
        CExperimentLogger::AddLine((std::to_string(temperature) + ";" + std::to_string(m_CurrentSolution->m_Fitness)).c_str());
        temperature *= m_CoolingRate;
    }
    
    CSOExperimentUtils::LogResultData(*m_CurrentSolution, m_Problem);
}

void CSA::InitializeSolution()
{
    SProblemEncoding& problemEncoding = m_Problem.GetProblemEncoding();
    m_CurrentSolution = m_Initialization.CreateSOIndividual(problemEncoding);
    m_Problem.Evaluate(*m_CurrentSolution);
    CAggregatedFitness::CountFitness(*m_CurrentSolution, m_ObjectiveWeights);
}

void CSA::Iterate(double temperature)
{
    SSOIndividual* neighborSolution = m_Initialization.CreateNeighborSolution(m_Problem.GetProblemEncoding(), *m_CurrentSolution);
    m_Problem.Evaluate(*neighborSolution);
    CAggregatedFitness::CountFitness(*neighborSolution, m_ObjectiveWeights);
    
    double delta = CAggregatedFitness::CalculateDelta(*neighborSolution, *m_CurrentSolution, m_ObjectiveWeights);

    if (delta < 0 || (exp(-delta / temperature) > CRandom::GetFloat(0.0f, 1.0f)))
    {
        delete m_CurrentSolution;
        m_CurrentSolution = neighborSolution;
    }
    else
    {
        delete neighborSolution;
    }
}

// Destructor to clean up
CSA::~CSA()
{

}
