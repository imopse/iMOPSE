#include "CTTPSingleFlip.h"
#include "utils/random/CRandom.h"
#include "method/operators/mutation/GenericMutationOperations.h"

void CTTPSingleFlip::Mutate(SProblemEncoding& problemEncoding, AIndividual& child)
{
    // Knapsack Mutation
    if (CRandom::GetFloat(0, 1) < m_SingleFlipMutProb)
    {
        GenericMutationOperations::FlipSingleRandomBit(child.m_Genotype.m_BoolGenotype);
    }
}
