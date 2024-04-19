#pragma once

#include <vector>
#include "SGenotype.h"

struct AIndividual
{
public:
    SGenotype m_Genotype;
    std::vector<float> m_Evaluation;
    std::vector<float> m_NormalizedEvaluation;
    bool m_isValid = true;
    
protected:
    AIndividual(SGenotype& genotype, std::vector<float>& evaluation, std::vector<float>& normalizedEvaluation)
        : m_Genotype(genotype)
        , m_Evaluation(evaluation)
        , m_NormalizedEvaluation(normalizedEvaluation)
    {};

    AIndividual(const AIndividual& other)
        : m_Genotype(other.m_Genotype)
        , m_Evaluation(other.m_Evaluation)
        , m_NormalizedEvaluation(other.m_NormalizedEvaluation)
    {};
};
