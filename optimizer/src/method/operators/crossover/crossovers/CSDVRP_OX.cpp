#include "CSDVRP_OX.h"

#include <algorithm>
#include <climits>
#include "utils/random/CRandom.h"


void CSDVRP_OX::Crossover(
    const SProblemEncoding &problemEncoding,
    AIndividual &firstParent,
    AIndividual &secondParent,
    AIndividual &firstChild,
    AIndividual &secondChild
) {
    for (const SEncodingSection &encoding: problemEncoding.m_Encoding) {
        const size_t sectionSize = encoding.m_SectionDescription.size();
        const auto &secondParentGenes = secondParent.m_Genotype;
        const auto &firstParentGenes = firstParent.m_Genotype;

        if (CRandom::GetFloat(0, 1) < m_CrossoverProbability) {
            int a = CRandom::GetInt(0, int(sectionSize) - 1);
            int b = CRandom::GetInt(0, int(sectionSize) - a) + a;

            std::fill(firstChild.m_Genotype.m_IntGenotype.begin(), firstChild.m_Genotype.m_IntGenotype.end(), -1);
            std::fill(secondChild.m_Genotype.m_IntGenotype.begin(), secondChild.m_Genotype.m_IntGenotype.end(), -1);

            std::copy(firstParentGenes.m_IntGenotype.begin() + a, firstParentGenes.m_IntGenotype.begin() + b,
                      secondChild.m_Genotype.m_IntGenotype.begin() + a);
            std::copy(secondParentGenes.m_IntGenotype.begin() + a, secondParentGenes.m_IntGenotype.begin() + b,
                      firstChild.m_Genotype.m_IntGenotype.begin() + a);

            int firstChildIdx = (b == sectionSize) ? 0 : b;
            int secondChildIdx = (b == sectionSize) ? 0 : b;

            for (int i = 0; i < sectionSize; ++i) {
                if (i < a || i >= b) {
                    while (std::find(firstChild.m_Genotype.m_IntGenotype.begin(),
                                     firstChild.m_Genotype.m_IntGenotype.end(),
                                     secondParentGenes.m_IntGenotype[firstChildIdx]) !=
                           firstChild.m_Genotype.m_IntGenotype.end() && secondParentGenes.m_IntGenotype[firstChildIdx]
                           != INT_MAX) {
                        firstChildIdx = (firstChildIdx + 1) % sectionSize;
                    }
                    firstChild.m_Genotype.m_IntGenotype[i] = secondParentGenes.m_IntGenotype[firstChildIdx];
                    firstChildIdx = (firstChildIdx + 1) % sectionSize;

                    while (std::find(secondChild.m_Genotype.m_IntGenotype.begin(),
                                     secondChild.m_Genotype.m_IntGenotype.end(),
                                     firstParentGenes.m_IntGenotype[secondChildIdx]) !=
                           secondChild.m_Genotype.m_IntGenotype.end() && firstParentGenes.m_IntGenotype[secondChildIdx]
                           != INT_MAX) {
                        secondChildIdx = (secondChildIdx + 1) % sectionSize;
                    }
                    secondChild.m_Genotype.m_IntGenotype[i] = firstParentGenes.m_IntGenotype[secondChildIdx];
                    secondChildIdx = (secondChildIdx + 1) % sectionSize;
                }
            }
        }
    }
}
