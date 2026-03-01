#pragma once

#include "method/multiOperator/AMultiOperator.h"
#include "utils/random/CRandom.h"

template <typename O>
class CMaxCreditByCallsMultiOperator: public AMultiOperator<O>
{
public:
    virtual CAtomicOperator<O>* SelectOperator()
    {
        if (m_AtomicOperators.empty())
        {
            return nullptr;
        }
        else
        {
            float bestVal = 0.f;
            CAtomicOperator<O>* bestOperator = nullptr;

            for (CAtomicOperator<O>& atomicOperator : m_AtomicOperators)
            {
                const auto& operatorData = atomicOperator.GetData();
                float operatorScore = ((float)operatorData.m_Credits + 1.f) / ((float)operatorData.m_Calls + 1.f);
                if (operatorScore > bestVal)
                {
                    bestVal = operatorScore;
                    bestOperator = &atomicOperator;
                }
            }
            return bestOperator;
        }
    }
};


