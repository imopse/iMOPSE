#include "CTTPFlip.h"
#include "method/operators/mutation/GenericMutationOperations.h"

void CTTPFlip::Mutate(SProblemEncoding& problemEncoding, AIndividual& child)
{
    // Knapsack Mutation
    GenericMutationOperations::RandomlyFlipBits(child.m_Genotype.m_BoolGenotype, m_FlipMutProb);
}
