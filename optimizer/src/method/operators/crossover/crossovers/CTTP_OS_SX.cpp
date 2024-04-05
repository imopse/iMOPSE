#include "CTTP_OS_SX.h"
#include "../../../../utils/random/CRandom.h"
#include <algorithm>

void CTTP_OS_SX::Crossover(const SProblemEncoding& problemEncoding, AIndividual &firstParent, AIndividual &secondParent,
                           AIndividual &firstChild, AIndividual &secondChild)
{
    for (const SEncodingSection &encoding: problemEncoding.m_Encoding)
    {
        const size_t sectionSize = encoding.m_SectionDescription.size();
        const auto &secondParentGenes = secondParent.m_Genotype;
        const auto &firstParentGenes = firstParent.m_Genotype;
        if (encoding.m_SectionType == EEncodingType::PERMUTATION)
        {
            // Route Crossover
            if (CRandom::GetFloat(0, 1) < m_RouteCrProb)
            {
                int a = CRandom::GetInt(0, int(sectionSize) - 1);
                int b = CRandom::GetInt(0, int(sectionSize) - a) + a;

                const auto &firstParentA = firstParentGenes.m_IntGenotype.begin() + a;
                const auto &firstParentB = firstParentGenes.m_IntGenotype.begin() + b;
                const auto &secParentA = secondParentGenes.m_IntGenotype.begin() + a;
                const auto &secParentB = secondParentGenes.m_IntGenotype.begin() + b;

                size_t firstSeekIdx = b;
                size_t secSeekIdx = b;

                for (size_t i = b; i != a; i = (i + 1) % sectionSize)
                {
                    // Update first child
                    while (std::find(firstParentA, firstParentB, secondParentGenes.m_IntGenotype[secSeekIdx]) !=
                           firstParentB)
                    {
                        secSeekIdx = (secSeekIdx + 1) % sectionSize;
                    }
                    firstChild.m_Genotype.m_IntGenotype[i] = secondParentGenes.m_IntGenotype[secSeekIdx];
                    secSeekIdx = (secSeekIdx + 1) % sectionSize;


                    // Update second child
                    while (std::find(secParentA, secParentB, firstParentGenes.m_IntGenotype[firstSeekIdx]) !=
                           secParentB)
                    {
                        firstSeekIdx = (firstSeekIdx + 1) % sectionSize;
                    }
                    secondChild.m_Genotype.m_IntGenotype[i] = firstParentGenes.m_IntGenotype[firstSeekIdx];
                    firstSeekIdx = (firstSeekIdx + 1) % sectionSize;
                }
            }
        }
        else if (encoding.m_SectionType == EEncodingType::BINARY)
        {
            // Knapsack Crossover
            if (CRandom::GetFloat(0, 1) < m_KnapCrProb)
            {
                size_t point = CRandom::GetInt(0, int(sectionSize));

                for (size_t g = 0; g < sectionSize; ++g)
                {
                    if (g < point)
                    {
                        firstChild.m_Genotype.m_BoolGenotype[g] = firstParentGenes.m_BoolGenotype[g];
                        secondChild.m_Genotype.m_BoolGenotype[g] = secondParentGenes.m_BoolGenotype[g];
                    }
                    else
                    {
                        firstChild.m_Genotype.m_BoolGenotype[g] = secondParentGenes.m_BoolGenotype[g];
                        secondChild.m_Genotype.m_BoolGenotype[g] = firstParentGenes.m_BoolGenotype[g];
                    }
                }
            }
        }
    }
}
