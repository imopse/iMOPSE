

#include "CRankedTournament.h"
#include "../../../../utils/random/CRandom.h"
#include <cstddef>

SMOIndividual *CRankedTournament::Select(std::vector<SMOIndividual *> &population)
{
    std::size_t popSize = population.size();

    std::size_t bestIdx = CRandom::GetInt(0, popSize);
    std::size_t bestRank = population[bestIdx]->m_Rank;

    for (std::size_t i = 1; i < m_TournamentSize; ++i)
    {
        std::size_t randomIdx = CRandom::GetInt(0, popSize);
        std::size_t rank = population[randomIdx]->m_Rank;
        if (rank < bestRank)
        {
            bestRank = rank;
            bestIdx = randomIdx;
        }
    }

    return population[bestIdx];
}
