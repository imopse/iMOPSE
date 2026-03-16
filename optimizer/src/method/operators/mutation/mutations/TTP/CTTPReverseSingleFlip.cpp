#include "CTTPReverseSingleFlip.h"
#include "utils/random/CRandom.h"
#include "method/operators/mutation/GenericMutationOperations.h"

void CTTPReverseSingleFlip::Mutate(SProblemEncoding& problemEncoding, AIndividual& child)
{
    // Route Mutation
    if (CRandom::GetFloat(0, 1) < m_ReverseMutProb)
    {
        GenericMutationOperations::InverseRandomPermutationSubsection(child.m_Genotype.m_IntGenotype);
    }
    // Knapsack Mutation
    if (CRandom::GetFloat(0, 1) < m_SingleFlipMutProb)
    {
        GenericMutationOperations::FlipSingleRandomBit(child.m_Genotype.m_BoolGenotype);
    }
}
