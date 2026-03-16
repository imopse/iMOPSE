#include <algorithm>
#include "GenericMutationOperations.h"
#include "utils/random/CRandom.h"

void GenericMutationOperations::InverseRandomPermutationSubsection(std::vector<int>& genotype)
{
    int firstGene = CRandom::GetInt(0, int(genotype.size()));
    int secondGene = CRandom::GetInt(0, int(genotype.size()));

    if (firstGene < secondGene)
    {
        std::reverse(genotype.begin() + firstGene, genotype.begin() + secondGene + 1);
    }
    else if (secondGene < firstGene)
    {
        std::reverse(genotype.begin() + secondGene, genotype.begin() + firstGene + 1);
    }
}


void GenericMutationOperations::RandomlySwapGenes(std::vector<int>& genotype, float prob)
{
    for (size_t i = 0; i < genotype.size(); ++i)
    {
        if (CRandom::GetFloat(0, 1) < prob)
        {
            int otherIdx = CRandom::GetInt(0, int(genotype.size()));
            std::swap(genotype[i], genotype[otherIdx]);
        }
    }
}

void GenericMutationOperations::FlipSingleRandomBit(std::vector<bool>& genotype)
{
    if (!genotype.empty())
    {
        int randItemIdx = CRandom::GetInt(0, int(genotype.size()));
        genotype[randItemIdx] = !genotype[randItemIdx];
    }
}

void GenericMutationOperations::RandomlyFlipBits(std::vector<bool>& genotype, float prob)
{
    for (size_t i = 0; i < genotype.size(); ++i)
    {
        if (CRandom::GetFloat(0, 1) < prob)
        {
            genotype[i] = !genotype[i];
        }
    }
}

void GenericMutationOperations::RandomlySetBitsValue(std::vector<bool>& genotype, float prob, bool val)
{
    for (size_t i = 0; i < genotype.size(); ++i)
    {
        if (CRandom::GetFloat(0, 1) < prob)
        {
            genotype[i] = val;
        }
    }
}