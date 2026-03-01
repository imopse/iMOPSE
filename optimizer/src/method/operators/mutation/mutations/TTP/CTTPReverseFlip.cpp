#include "CTTPReverseFlip.h"
#include "utils/random/CRandom.h"
#include "method/operators/mutation/GenericMutationOperations.h"

void CTTPReverseFlip::Mutate(SProblemEncoding& problemEncoding, AIndividual& child)
{
    // Route Mutation
    if (CRandom::GetFloat(0, 1) < m_ReverseMutProb)
    {
        GenericMutationOperations::InverseRandomPermutationSubsection(child.m_Genotype.m_IntGenotype);
    }
    // Knapsack Mutation
    GenericMutationOperations::RandomlyFlipBits(child.m_Genotype.m_BoolGenotype, m_FlipMutProb);
}
