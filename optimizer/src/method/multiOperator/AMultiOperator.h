#pragma once

#include <vector>
#include "CAtomicOperator.h"

template <typename O>
class AMultiOperator
{
public:
    void AddOperator(O* newAtomicOperator)
    {
        m_AtomicOperators.emplace_back(newAtomicOperator, m_AtomicOperators.size());
    }

    virtual ~AMultiOperator()
    {
        for (CAtomicOperator<O>& atomicOperator : m_AtomicOperators)
        {
            delete atomicOperator.Get();
        }
    }

    void ResetAllOperatorData()
    {
        for (auto& atomicOperator : m_AtomicOperators)
        {
            atomicOperator.ResetData();
        }
    }

    SAtomicOperatorData& GetOperatorData(size_t operatorId) { return m_AtomicOperators[operatorId].GetData(); }

    // Abstract function to override
    virtual CAtomicOperator<O>* SelectOperator() = 0;

protected:
    std::vector<CAtomicOperator<O>> m_AtomicOperators;
};
