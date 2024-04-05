#pragma once

#include "../ACrossover.h"

class CUniformCX : public ACrossover
{
public:
    explicit CUniformCX(float crossoverProbability) : m_CrossoverProbability(crossoverProbability)
    {};
    ~CUniformCX() override = default;

    void Crossover(
            const SProblemEncoding& problemEncoding,
            AIndividual &firstParent,
            AIndividual &secondParent,
            AIndividual &firstChild,
            AIndividual &secondChild) override;
private:
    float m_CrossoverProbability;
};
