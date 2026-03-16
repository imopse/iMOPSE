#include "CTTPSwapFlip.h"
#include "method/operators/mutation/GenericMutationOperations.h"

void CTTPSwapFlip::Mutate(SProblemEncoding& problemEncoding, AIndividual& child)
{
    // Route Mutation
    GenericMutationOperations::RandomlySwapGenes(child.m_Genotype.m_IntGenotype, m_GeneSwapProb);
    // Knapsack Mutation
    GenericMutationOperations::RandomlyFlipBits(child.m_Genotype.m_BoolGenotype, m_FlipMutProb);
}
