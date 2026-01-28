#pragma once

#include "method/operators/crossover/ACrossover.h"
#include <stdexcept>

class CSDVRP_OX : public ACrossover {
public:
    explicit CSDVRP_OX(float crossoverProbability) : m_CrossoverProbability(crossoverProbability) {
    };

    ~CSDVRP_OX() override = default;

    void Crossover(
        const SProblemEncoding &problemEncoding,
        AIndividual &firstParent,
        AIndividual &secondParent,
        AIndividual &firstChild,
        AIndividual &secondChild) override;

private:
    float m_CrossoverProbability;
};
