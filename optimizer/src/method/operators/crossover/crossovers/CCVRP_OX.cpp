#include "../../../../utils/random/CRandom.h"
#include <algorithm>
#include <climits>
#include "CCVRP_OX.h"

void CCVRP_OX::FixChild(AIndividual &child) {
    auto &childGenes = child.m_Genotype.m_IntGenotype;
    auto size = child.m_Genotype.m_IntGenotype.size();


    std::vector<int> all_genes(size);
    for (int i = 0; i < child.m_Genotype.m_IntGenotype.size(); i++) {
        all_genes[i] = i;
    }

    std::vector<int> missing_genes;
    for (auto gene: all_genes) {
        if (std::count(childGenes.begin(), childGenes.end(), gene) < 1) {
            missing_genes.push_back(gene);
        }
    }

    int ii = 0;
    for (size_t i = 0; i < childGenes.size(); ++i) {
        int gene = childGenes[i];

        if (std::count(childGenes.begin(), childGenes.end(), gene) > 1) {
            auto it = std::find(all_genes.begin(), all_genes.end(), gene);

            if (it != all_genes.end()) {
                size_t index = std::distance(childGenes.begin(), it);
                child.m_Genotype.m_IntGenotype[i] = missing_genes[ii];
                ii++;
            }
        }
    }

    missing_genes.clear();
    for (auto gene: all_genes) {
        if (std::count(childGenes.begin(), childGenes.end(), gene) < 1) {
            missing_genes.push_back(gene);
        }
    }

}

void CCVRP_OX::Crossover(const SProblemEncoding& problemEncoding, AIndividual &firstParent, AIndividual &secondParent,
                         AIndividual &firstChild, AIndividual &secondChild) {
    for (const SEncodingSection &encoding: problemEncoding.m_Encoding) {
        const size_t sectionSize = encoding.m_SectionDescription.size();
        const auto &secondParentGenes = secondParent.m_Genotype;
        const auto &firstParentGenes = firstParent.m_Genotype;

        // Order Crossover (OX1)
        if (CRandom::GetFloat(0, 1) < m_CrossoverProbability) {
            int a = CRandom::GetInt(0, int(sectionSize) - 1);
            int b = CRandom::GetInt(0, int(sectionSize) - a) + a;

            std::fill(firstChild.m_Genotype.m_IntGenotype.begin(), firstChild.m_Genotype.m_IntGenotype.end(), -1);
            std::fill(secondChild.m_Genotype.m_IntGenotype.begin(), secondChild.m_Genotype.m_IntGenotype.end(), -1);

            // Copy the selected range from the parents to the children
            std::copy(firstParentGenes.m_IntGenotype.begin() + a, firstParentGenes.m_IntGenotype.begin() + b,
                      secondChild.m_Genotype.m_IntGenotype.begin() + a);
            std::copy(secondParentGenes.m_IntGenotype.begin() + a, secondParentGenes.m_IntGenotype.begin() + b,
                      firstChild.m_Genotype.m_IntGenotype.begin() + a);

            // Update remaining positions in children using the order of the other parent
            int firstChildIdx = (b == sectionSize) ? 0 : b;
            int secondChildIdx = (b == sectionSize) ? 0 : b;

            for (int i = 0; i < sectionSize; ++i) {
                if (i < a || i >= b) {
                    while (std::find(firstChild.m_Genotype.m_IntGenotype.begin(),
                                     firstChild.m_Genotype.m_IntGenotype.end(),
                                     secondParentGenes.m_IntGenotype[firstChildIdx]) !=
                           firstChild.m_Genotype.m_IntGenotype.end() && secondParentGenes.m_IntGenotype[firstChildIdx] != INT_MAX) {
                        firstChildIdx = (firstChildIdx + 1) % sectionSize;
                    }
                    firstChild.m_Genotype.m_IntGenotype[i] = secondParentGenes.m_IntGenotype[firstChildIdx];
                    firstChildIdx = (firstChildIdx + 1) % sectionSize;

                    while (std::find(secondChild.m_Genotype.m_IntGenotype.begin(),
                                     secondChild.m_Genotype.m_IntGenotype.end(),
                                     firstParentGenes.m_IntGenotype[secondChildIdx]) !=
                           secondChild.m_Genotype.m_IntGenotype.end() && firstParentGenes.m_IntGenotype[secondChildIdx] != INT_MAX) {
                        secondChildIdx = (secondChildIdx + 1) % sectionSize;
                    }
                    secondChild.m_Genotype.m_IntGenotype[i] = firstParentGenes.m_IntGenotype[secondChildIdx];
                    secondChildIdx = (secondChildIdx + 1) % sectionSize;
                }
            }
        }
    }
}
