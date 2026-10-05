#include "CFitnessTournament.h"
#include "../../../../utils/random/CRandom.h"
#include <cstddef>

SSOIndividual *CFitnessTournament::Select(std::vector<SSOIndividual *> &population)
{
    std::size_t bestIdx = CRandom::GetInt(0, population.size());
    float bestFitness = population[bestIdx]->m_Fitness;

    for (std::size_t i = 0; i < m_TournamentSize; i++)
    {
        std::size_t randomIdx = CRandom::GetInt(0, population.size());
        float currentFitness = population[randomIdx]->m_Fitness;

        if (currentFitness < bestFitness)
        {
            bestFitness = currentFitness;
            bestIdx = randomIdx;
        }
    }

    return population[bestIdx];
}
