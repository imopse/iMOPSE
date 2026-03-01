#include "CTTPDropItems.h"
#include "method/operators/mutation/GenericMutationOperations.h"

void CTTPDropItems::Mutate(SProblemEncoding& problemEncoding, AIndividual& child)
{
    // Knapsack Mutation
    GenericMutationOperations::RandomlySetBitsValue(child.m_Genotype.m_BoolGenotype, m_DropItemProb, false);
}
