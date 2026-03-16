#pragma once

#include <vector>

namespace GenericMutationOperations
{
    void InverseRandomPermutationSubsection(std::vector<int>& genotype);
    void RandomlySwapGenes(std::vector<int>& genotype, float prob);
    void FlipSingleRandomBit(std::vector<bool>& genotype);
    void RandomlyFlipBits(std::vector<bool>& genotype, float prob);
    void RandomlySetBitsValue(std::vector<bool>& genotype, float prob, bool val);
}
