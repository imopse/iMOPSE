

#include "CRankedTournament.h"
#include "../../../../utils/random/CRandom.h"

SMOIndividual *CRankedTournament::Select(std::vector<SMOIndividual *> &population)
{
    size_t popSize = population.size();

    size_t bestIdx = CRandom::GetInt(0, popSize);
    size_t bestRank = population[bestIdx]->m_Rank;

    for (size_t i = 1; i < m_TournamentSize; ++i)
    {
        size_t randomIdx = CRandom::GetInt(0, popSize);
        size_t rank = population[randomIdx]->m_Rank;
        if (rank < bestRank)
        {
            bestRank = rank;
            bestIdx = randomIdx;
        }
    }

    return population[bestIdx];
}
