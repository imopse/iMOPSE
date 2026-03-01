#include "CMSRA_RX.h"
#include "../../../../utils/random/CRandom.h"
#include <algorithm>
#include "problem/problems/MSRA/MSRAProblem.h"

void CMSRA_RX::Crossover(const SProblemEncoding& problemEncoding, AIndividual &firstParent, AIndividual &secondParent,
                           AIndividual &firstChild, AIndividual &secondChild)
{
    if (CRandom::GetFloat(0.0f, 1.0f) < m_ResourceCrProb)
    {
        const size_t sectionSize = problemEncoding.m_Encoding[0].m_SectionDescription.size();
        size_t stageCount = m_ProblemDefinition.GetStageCount();

        auto& firstChildGenes = firstChild.m_Genotype.m_FloatGenotype;
        auto& secondChildGenes = secondChild.m_Genotype.m_FloatGenotype;

        auto& firstParentGenes = firstParent.m_Genotype.m_FloatGenotype;
        auto& secondParentGenes = secondParent.m_Genotype.m_FloatGenotype;

        // copy all stages for the resource
        for (size_t g = 0; g < sectionSize; g += stageCount)
        {
            if (CRandom::GetFloat(0.0f, 1.0f) < 0.5f)
            {
                std::copy(secondParentGenes.begin() + g, secondParentGenes.begin() + g + stageCount,
                          firstChildGenes.begin() + g);
            }
            if (CRandom::GetFloat(0.0f, 1.0f) < 0.5f)
            {
                std::copy(firstParentGenes.begin() + g, firstParentGenes.begin() + g + stageCount,
                          secondChildGenes.begin() + g);
            }
        }
    }
}
