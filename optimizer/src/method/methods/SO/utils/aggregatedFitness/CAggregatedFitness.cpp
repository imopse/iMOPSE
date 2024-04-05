
#include "CAggregatedFitness.h"

void CAggregatedFitness::CountFitness(SSOIndividual &individual, std::vector<float> &objectiveWeights)
{
    if (objectiveWeights.empty() || objectiveWeights.size() == 1)
    {
        individual.m_Fitness = individual.m_NormalizedEvaluation[0];
    }

    float fitness = 0.0f;
    for (int i = 0; i < objectiveWeights.size(); ++i)
    {
        fitness += individual.m_NormalizedEvaluation[i] * objectiveWeights[i];
    }

    individual.m_Fitness = fitness;
}

double CAggregatedFitness::CalculateDelta(const SSOIndividual& newSolution, const SSOIndividual& currentSolution, std::vector<float> &objectiveWeights)
{
    double delta = 0.0;
    for (size_t i = 0; i < newSolution.m_NormalizedEvaluation.size(); ++i)
    {
        double weight = (i < objectiveWeights.size()) ? objectiveWeights[i] : 1.0;
        delta += weight * (newSolution.m_NormalizedEvaluation[i] - currentSolution.m_NormalizedEvaluation[i]);
    }
    return delta;
}