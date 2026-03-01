#pragma once

#include "method/operators/mutation/AMutation.h"

class CTTPReverse : public AMutation
{
public:
    explicit CTTPReverse(float reverseMutProb) :
        m_ReverseMutProb(reverseMutProb)
    {};
    ~CTTPReverse() override = default;

    void Mutate(SProblemEncoding& problemEncoding, AIndividual& child) override;

    size_t GetParamCount() const override { return 0; }
    float* GetParamValue(int paramIdx) override { return paramIdx == 0 ? &m_ReverseMutProb : nullptr; }

private:
    float m_ReverseMutProb;
};
