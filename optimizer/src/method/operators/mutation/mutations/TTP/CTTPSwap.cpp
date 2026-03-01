#include "CTTPSwap.h"
#include "method/operators/mutation/GenericMutationOperations.h"

void CTTPSwap::Mutate(SProblemEncoding& problemEncoding, AIndividual& child)
{
    // Route Mutation
    GenericMutationOperations::RandomlySwapGenes(child.m_Genotype.m_IntGenotype, m_GeneSwapProb);
}
