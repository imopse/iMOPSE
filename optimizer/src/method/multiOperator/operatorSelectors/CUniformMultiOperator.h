#pragma once

#include "method/multiOperator/AMultiOperator.h"
#include "utils/random/CRandom.h"

template <typename O>
class CUniformMultiOperator: public AMultiOperator<O>
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
            return &m_AtomicOperators[CRandom::GetInt(0, (int)m_AtomicOperators.size())];
        }
    }
};

