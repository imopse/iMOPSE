#include "CTTPSwapSingleFlip.h"
#include "utils/random/CRandom.h"
#include "method/operators/mutation/GenericMutationOperations.h"

void CTTPSwapSingleFlip::Mutate(SProblemEncoding& problemEncoding, AIndividual& child)
{
    // Route Mutation
    GenericMutationOperations::RandomlySwapGenes(child.m_Genotype.m_IntGenotype, m_GeneSwapProb);
    // Knapsack Mutation
    if (CRandom::GetFloat(0, 1) < m_SingleFlipMutProb)
    {
        GenericMutationOperations::FlipSingleRandomBit(child.m_Genotype.m_BoolGenotype);
    }
}
