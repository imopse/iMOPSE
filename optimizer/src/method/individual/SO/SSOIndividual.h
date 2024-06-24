
#pragma once

#include <vector>
#include "../AIndividual.h"

struct SSOIndividual : public AIndividual
{
public:
    SSOIndividual(SGenotype &genotype, std::vector<float>& evaluation, std::vector<float>& normalizedEvaluation)
            : AIndividual(genotype, evaluation, normalizedEvaluation)
    {}

    SSOIndividual(const SSOIndividual &other) : AIndividual(other)
    {};

    SSOIndividual(const AIndividual& other) : AIndividual(other)
    {};
    
    float m_Fitness = 0;
};
