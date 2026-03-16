#pragma once

#include "method/multiOperator/AMultiOperator.h"

template <typename O>
class CUCB2MultiOperator: public AMultiOperator<O>
{
public:
    virtual CAtomicOperator<O>* SelectOperator()
    {
        if (this->m_AtomicOperators.empty())
        {
            return nullptr;
        }
        else
        {
            float m_TotalCalls = 0.f;
            float m_TotalCredits = 0.000001f; // so to make sure we do not divide by 0

            for (CAtomicOperator<O>& atomicOperator : this->m_AtomicOperators)
            {
                const auto& operatorData = atomicOperator.GetData();
                m_TotalCalls += (float)operatorData.m_Calls;
                m_TotalCredits += (float)operatorData.m_Credits;
            }

            float bestVal = -FLT_MAX;
            //float lambda = 5.f;
            //float lambda = 3.f;
            float lambda = 1.f;
            //float lambda = sqrtf(2.f);
            //float lambda = 1.f / sqrtf(2.f);
            //float lambda = 0.5f;
            //float lambda = 0.1f;
            CAtomicOperator<O>* bestOperator = nullptr;

            for (CAtomicOperator<O>& atomicOperator : this->m_AtomicOperators)
            {
                const auto& operatorData = atomicOperator.GetData();
                float operatorScore = FLT_MAX;
                if (operatorData.m_Calls >= 1.f)
                {
                    //float creditScore = ((float)operatorData.m_Credits / (float)operatorData.m_Calls); // alt
                    float creditScore = ((float)operatorData.m_Credits / m_TotalCredits); // TODO - verify this variant on other methods if possible
                    float callsScore = sqrtf(2 * logf(m_TotalCalls) / ((float)operatorData.m_Calls));
                    operatorScore = creditScore + lambda * callsScore;
                }
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


