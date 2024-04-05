#pragma once
#include "../../individual/SO/SSOIndividual.h"


class CAggregatedFitness
{
public:
    static void CountFitness(SSOIndividual &individual, std::vector<float> &objectiveWeights);
};
