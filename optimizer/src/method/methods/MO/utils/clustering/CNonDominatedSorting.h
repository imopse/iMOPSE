#pragma once

#include "../../../../individual/MO/SMOIndividual.h"
#include <cstddef>

class CNonDominatedSorting
{
public:

    void Cluster(std::vector<SMOIndividual *> &population, std::vector<std::vector<std::size_t>> &clusters);

private:

    struct SSolution
    {
        SSolution(std::size_t i);

        std::size_t m_Idx;
        std::vector<SSolution *> m_DominatedSolutions;
        std::size_t m_DominationCounter;
    };
};
