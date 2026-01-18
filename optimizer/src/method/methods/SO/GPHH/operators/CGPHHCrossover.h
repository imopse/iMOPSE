#pragma once

#include "../../../../operators/crossover/ACrossover.h"
#include "../GPHHUtils.h"
#include "../individual/CGPHHIndividual.h"

class CGPHHCrossover : public ACrossover {
public:
    CGPHHCrossover(int maxDepth, float crossoverRate);
    
    void Crossover(
            const SProblemEncoding& problemEncoding,
            AIndividual &firstParent,
            AIndividual &secondParent,
            AIndividual &firstChild,
            AIndividual &secondChild) override;

private:
    int m_MaxDepth;
    float m_CrossoverRate;
};
