#include "CSDVRPSwap.h"

#include "utils/random/CRandom.h"

void CSDVRPSwap::Mutate(SProblemEncoding &problemEncoding, AIndividual &child) {
    size_t genomeSize = child.m_Genotype.m_IntGenotype.size();
    for (size_t i = 0; i < genomeSize; ++i) {
        for (size_t j = i + 1; j < genomeSize; ++j) {
            if (CRandom::GetFloat(0, 1) < m_mutationProbability) {
                std::swap(child.m_Genotype.m_IntGenotype[i], child.m_Genotype.m_IntGenotype[j]);
            }
        }
    }
}
