#include "CNonDominatedSorting.h"

void CNonDominatedSorting::Cluster(std::vector<SMOIndividual *> &population, std::vector<std::vector<size_t>> &clusters)
{
    size_t popSize = population.size();
    std::vector<SSolution> solutions;
    solutions.reserve(popSize);
    for (size_t i = 0; i < popSize; ++i)
    {
        solutions.emplace_back(i);
    }

    // clusters == fronts
    clusters.push_back(std::vector<size_t>());

    for (size_t p = 0; p < popSize; ++p)
    {
        for (size_t q = 0; q < popSize; ++q)
        {
            if (p != q)
            {
                if (population[q]->IsDominatedBy(population[p]))
                {
                    solutions[p].m_DominatedSolutions.push_back(&(solutions[q]));
                }
                else if (population[p]->IsDominatedBy(population[q]))
                {
                    solutions[p].m_DominationCounter += 1;
                }
            }
        }
        if (solutions[p].m_DominationCounter == 0)
        {
            clusters[0].push_back(p);
        }
    }

    size_t rank = 1;
    while (!clusters[rank - 1].empty())
    {
        clusters.push_back(std::vector<size_t>());
        for (size_t solutionIdx: clusters[rank - 1])
        {
            for (SSolution *dominatedSolution: solutions[solutionIdx].m_DominatedSolutions)
            {
                dominatedSolution->m_DominationCounter -= 1;
                if (dominatedSolution->m_DominationCounter == 0)
                {
                    clusters[rank].push_back(dominatedSolution->m_Idx);
                }
            }
        }
        ++rank;
    }

    // Remove last, which is empty
    clusters.pop_back();

    // Assign ranks to population
    for (size_t c = 0; c < clusters.size(); ++c)
    {
        for (size_t i = 0; i < clusters[c].size(); ++i)
        {
            population[clusters[c][i]]->m_Rank = c;
        }
    }
}

CNonDominatedSorting::SSolution::SSolution(size_t i)
        : m_Idx(i), m_DominationCounter(0)
{
}
