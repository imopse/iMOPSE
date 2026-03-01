#include "CTTPPickItems.h"
#include "method/operators/mutation/GenericMutationOperations.h"

void CTTPPickItems::Mutate(SProblemEncoding& problemEncoding, AIndividual& child)
{
    // Knapsack Mutation
    GenericMutationOperations::RandomlySetBitsValue(child.m_Genotype.m_BoolGenotype, m_PickItemProb, true);
}
