#pragma once

#include "method/multiOperator/AMultiOperator.h"
#include "utils/random/CRandom.h"

template <typename O>
class CInvCreditRouletteMultiOperator: public AMultiOperator<O>
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
            float invCreditSum = 0.f;
            for (CAtomicOperator<O>& atomicOperator : this->m_AtomicOperators)
            {
                invCreditSum += (1.f / ((float)atomicOperator.GetData().m_Credits + 1));
            }
            float randVal = CRandom::GetFloat(0.f, 1.f);
            float rouletteWheel = 0.f;
            for (CAtomicOperator<O>& atomicOperator : this->m_AtomicOperators)
            {
                rouletteWheel += ((1.f / ((float)atomicOperator.GetData().m_Credits + 1)) / invCreditSum);
                if (randVal < rouletteWheel)
                {
                    return &atomicOperator;
                }
            }
            return &this->m_AtomicOperators.back();
        }
    }
};


