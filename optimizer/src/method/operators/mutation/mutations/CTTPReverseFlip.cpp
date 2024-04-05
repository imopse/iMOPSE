#include <algorithm>
#include "CTTPReverseFlip.h"
#include "utils/random/CRandom.h"

void CTTPReverseFlip::Mutate(SProblemEncoding& problemEncoding, AIndividual &child)
{
    // Route Mutation
    if (CRandom::GetFloat(0, 1) < m_ReverseMutProb)
    {
        // Use inverse mutation for cities
        int firstGene = CRandom::GetInt(0, int(child.m_Genotype.m_IntGenotype.size()));
        int secondGene = CRandom::GetInt(0, int(child.m_Genotype.m_IntGenotype.size()));

        if (firstGene < secondGene)
            std::reverse(child.m_Genotype.m_IntGenotype.begin() + firstGene,
                         child.m_Genotype.m_IntGenotype.begin() + secondGene + 1);

        else if (secondGene < firstGene)
        {
            std::reverse(child.m_Genotype.m_IntGenotype.begin() + secondGene,
                         child.m_Genotype.m_IntGenotype.begin() + firstGene + 1);
        }
    }
    // Knapsack Mutation
    if (child.m_Genotype.m_BoolGenotype.size() != 0 && CRandom::GetFloat(0, 1) < m_FlipMutProb)
    {
        int randItemIdx = CRandom::GetInt(0, int(child.m_Genotype.m_BoolGenotype.size()));
        child.m_Genotype.m_BoolGenotype[randItemIdx] = !child.m_Genotype.m_BoolGenotype[randItemIdx];
    }
}
