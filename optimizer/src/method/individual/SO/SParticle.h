#pragma once

#include <vector>
#include "SSOIndividual.h"

struct SParticle : public SSOIndividual
{
public:
    SParticle(SGenotype &genotype, std::vector<float>& evaluation, std::vector<float>& normalizedEvaluation, std::vector<float>& velocity)
        : SSOIndividual(genotype, evaluation, normalizedEvaluation), m_Velocity(velocity)
    {}
    
    std::vector<float> m_Velocity;
    std::vector<float> m_BestKnownPosition;
    float m_BestKnownFitness = 0;
};