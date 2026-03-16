#include "CTTPFullOpt2.h"
#include <algorithm>
#include "problem/problems/TTP/CTTP2.h"

void CTTPFullOpt2::Mutate(SProblemEncoding& problemEncoding, AIndividual& child)
{
    // Route Greedy Search
    std::vector<int>& genotype = child.m_Genotype.m_IntGenotype;
    size_t citiesSize = genotype.size();
    const auto& distMtx = m_ProblemDefinition.GetDistMtx();

    for (size_t i = 0; i < citiesSize - 1; ++i)
    {
        const int geneI = genotype[i];
        const int geneIplus = genotype[(i + 1) % citiesSize];
        for (size_t j = i + 1; j < citiesSize; ++j)
        {
            const int geneJ = genotype[j];
            const int geneJplus = genotype[(j + 1) % citiesSize];
            float lengthDelta = (distMtx[geneI][geneJ] + distMtx[geneIplus][geneJplus]) - (distMtx[geneI][geneIplus] + distMtx[geneJ][geneJplus]);
            if (lengthDelta < 0)
            {
                std::reverse(genotype.begin() + i + 1, genotype.begin() + j + 1);
            }
        }
    }
}
