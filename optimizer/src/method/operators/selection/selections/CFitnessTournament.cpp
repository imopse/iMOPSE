#include "CFitnessTournament.h"
#include "../../../../utils/random/CRandom.h"

SSOIndividual *CFitnessTournament::Select(std::vector<SSOIndividual *> &population)
{
    size_t bestIdx = CRandom::GetInt(0, population.size());
    float bestFitness = population[bestIdx]->m_Fitness;

    for (size_t i = 0; i < m_TournamentSize; i++)
    {
        size_t randomIdx = CRandom::GetInt(0, population.size());
        float currentFitness = population[randomIdx]->m_Fitness;

        if (currentFitness < bestFitness)
        {
            bestFitness = currentFitness;
            bestIdx = randomIdx;
        }
    }

    return population[bestIdx];
}