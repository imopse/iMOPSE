#include "CTTPReverse.h"
#include "utils/random/CRandom.h"
#include "method/operators/mutation/GenericMutationOperations.h"

void CTTPReverse::Mutate(SProblemEncoding& problemEncoding, AIndividual& child)
{
    // Route Mutation
    if (CRandom::GetFloat(0, 1) < m_ReverseMutProb)
    {
        GenericMutationOperations::InverseRandomPermutationSubsection(child.m_Genotype.m_IntGenotype);
    }
}
