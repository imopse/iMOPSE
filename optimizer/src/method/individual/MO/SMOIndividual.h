#pragma once

#include <vector>
#include <cstddef>
#include <optional>
#include "../AIndividual.h"
#include "method/multiOperator/SAtomicOperatorData.h"

class SMOIndividual : public AIndividual
{
public:
    SMOIndividual(SGenotype& genotype, std::vector<float>& evaluation, std::vector<float>& normalizedEvaluation)
        : AIndividual(genotype, evaluation, normalizedEvaluation)
    {}

    SMOIndividual(const SMOIndividual& other)
        : AIndividual(other)
        , m_MetaInfo(other.m_MetaInfo)
        , m_MutationIdx(other.m_MutationIdx)
        , m_OperatorsData(other.m_OperatorsData) // testing
    {};
    
    bool IsDominatedBy(const SMOIndividual* otherSolution) const
    {
        // If this has any better value
        for (int i = 0; i < m_NormalizedEvaluation.size(); ++i)
        {
            if (m_NormalizedEvaluation[i] < otherSolution->m_NormalizedEvaluation[i])
            {
                return false;
            }
        }

        // Now we are sure we have worse or equal values

        // If other has at least one better value
        for (int i = 0; i < m_NormalizedEvaluation.size(); ++i)
        {
            if (otherSolution->m_NormalizedEvaluation[i] < m_NormalizedEvaluation[i])
            {
                return true;
            }
        }

        // If equal
        return false;
    }

    bool IsDuplicateEvalValue(const SMOIndividual* otherSolution) const
    {
        for (int i = 0; i < m_NormalizedEvaluation.size(); ++i)
        {
            if (otherSolution->m_NormalizedEvaluation[i] != m_NormalizedEvaluation[i])
            {
                return false;
            }
        }

        return true;
    }

    size_t GetSelected() const
    {
        return m_Selected;
    }

    void OnSelected()
    {
        m_Selected += 1;
    }

    size_t m_Rank = 0;
    float m_CrowdingDistance = 0.0f;
    std::vector<float> m_MetaInfo;
    std::optional<size_t> m_MutationIdx = {};
    std::vector<size_t> m_MutationCounters;
    std::vector<SAtomicOperatorData> m_OperatorsData;
private:
    size_t m_Selected = 0;
};