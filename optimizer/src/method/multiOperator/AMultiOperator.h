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

    void MultAllOperatorDataBy(float m)
    {
        for (auto& atomicOperator : m_AtomicOperators)
        {
            //atomicOperator.GetData().m_AccCalls *= m;
            atomicOperator.GetData().m_Calls *= m;
            atomicOperator.GetData().m_Credits *= m;
        }
    }

    void ResetAllOperatorDataButAccCalls()
    {
        for (auto& atomicOperator : m_AtomicOperators)
        {
            size_t accCalls = atomicOperator.GetData().m_AccCalls;
            atomicOperator.ResetData();
            atomicOperator.GetData().m_AccCalls = accCalls;
        }
    }

    SAtomicOperatorData& GetOperatorData(size_t operatorId) { return m_AtomicOperators[operatorId].GetData(); }
    size_t GetOperatorCount() const { return m_AtomicOperators.size(); }

    std::vector<CAtomicOperator<O>>& GetAtomicOperators() { return m_AtomicOperators; }

    // Abstract function to override
    virtual CAtomicOperator<O>* SelectOperator() = 0;

protected:
    std::vector<CAtomicOperator<O>> m_AtomicOperators;
};
