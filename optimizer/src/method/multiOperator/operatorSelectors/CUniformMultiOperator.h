#pragma once

#include "method/multiOperator/AMultiOperator.h"
#include "utils/random/CRandom.h"

template <typename O>
class CUniformMultiOperator: public AMultiOperator<O>
{
public:
    CAtomicOperator<O>* SelectOperator()
    {
        if (this->m_AtomicOperators.empty())
        {
            return nullptr;
        }
        else
        {
            return &this->m_AtomicOperators[CRandom::GetInt(0, (int)this->m_AtomicOperators.size())];
        }
    }
};

