#pragma once

#include "SAtomicOperatorData.h"

template <typename O>
class CAtomicOperator
{
public:
    explicit CAtomicOperator(O* newAtomicOperator, size_t id)
        : m_Operator(newAtomicOperator)
        , m_Id(id)
    {}

    O* Get() const { return m_Operator; }
    size_t GetId() const { return m_Id; }
    SAtomicOperatorData& GetData() { return m_Data; }
    void ResetData() { m_Data = SAtomicOperatorData(); }

private:
    O* m_Operator = nullptr;
    size_t m_Id;
    SAtomicOperatorData m_Data;
};