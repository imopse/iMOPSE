#pragma once

#include "../ACrossover.h"

class CMSRAProblem;

class CMSRA_RX : public ACrossover
{
public:
    explicit CMSRA_RX(float resourceCrProb, const CMSRAProblem& problemDefinition)
        : m_ResourceCrProb(resourceCrProb)
        , m_ProblemDefinition(problemDefinition)
    {}
    ~CMSRA_RX() override = default;

    void Crossover(
        const SProblemEncoding& problemEncoding,
        AIndividual &firstParent,
        AIndividual &secondParent,
        AIndividual &firstChild,
        AIndividual &secondChild) override;
private:
    float m_ResourceCrProb;
    const CMSRAProblem& m_ProblemDefinition;
};