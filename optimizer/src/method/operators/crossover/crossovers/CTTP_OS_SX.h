#pragma once

#include "../ACrossover.h"

class CTTP_OS_SX : public ACrossover
{
public:
    explicit CTTP_OS_SX(float routeCrProb, float knapCrProb) : m_RouteCrProb(routeCrProb), m_KnapCrProb(knapCrProb)
    {};
    ~CTTP_OS_SX() override = default;

    void Crossover(
            const SProblemEncoding& problemEncoding,
            AIndividual &firstParent,
            AIndividual &secondParent,
            AIndividual &firstChild,
            AIndividual &secondChild) override;
private:
    float m_RouteCrProb;
    float m_KnapCrProb;
};