#include "CRandomBit.h"
#include "utils/random/CRandom.h"

void CRandomBit::Mutate(SProblemEncoding& problemEncoding, AIndividual &child)
{
    const size_t sectionSize = problemEncoding.m_Encoding[0].m_SectionDescription.size();
    for (size_t g = 0; g < sectionSize; ++g)
    {
        if (CRandom::GetFloat(0, 1) < m_MutationProbability)
        {
            float minValue = problemEncoding.m_Encoding[0].m_SectionDescription[g].m_MinValue;
            float maxValue = problemEncoding.m_Encoding[0].m_SectionDescription[g].m_MaxValue;
            child.m_Genotype.m_FloatGenotype[g] = CRandom::GetFloat(minValue, maxValue);
        }
    }
}
