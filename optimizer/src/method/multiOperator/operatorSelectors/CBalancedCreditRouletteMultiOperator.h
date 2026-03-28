#pragma once

#include "method/multiOperator/AMultiOperator.h"
#include "utils/random/CRandom.h"

template <typename O>
class CBalancedCreditRouletteMultiOperator: public AMultiOperator<O>
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
            float scoreSum = 0.f;
            for (CAtomicOperator<O>& atomicOperator :this-> m_AtomicOperators)
            {
                //scoreSum += ((float)atomicOperator.GetData().m_Credits + 1) / ((float)atomicOperator.GetData().m_Calls + 1);
                //scoreSum += ((float)atomicOperator.GetData().m_Credits + 1) / sqrtf((float)atomicOperator.GetData().m_Calls + 1);
                scoreSum += ((float)atomicOperator.GetData().m_Credits + 1) / (logf((float)atomicOperator.GetData().m_Calls + 1) + 1);
            }
            float randVal = CRandom::GetFloat(0.f, 1.f);
            float rouletteWheel = 0.f;
            for (CAtomicOperator<O>& atomicOperator : this->m_AtomicOperators)
            {
                rouletteWheel += (((float)atomicOperator.GetData().m_Credits + 1) / scoreSum);
                if (randVal < rouletteWheel)
                {
                    return &atomicOperator;
                }
            }
            return &this->m_AtomicOperators.back();
        }
    }
};


