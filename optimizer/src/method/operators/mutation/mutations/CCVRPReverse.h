//
#pragma once

#include "../AMutation.h"

class CCVRPReverse : public AMutation
{
public:
    explicit CCVRPReverse(float reverseMutProb) : m_ReverseMutProb(reverseMutProb)
    {};
    ~CCVRPReverse() override = default;

    void Mutate(SProblemEncoding& problemEncoding, AIndividual& child) override;

    size_t GetParamCount() const override { return 1; }
    float* GetParamValue(int paramIdx) override { return paramIdx == 0 ? &m_ReverseMutProb : nullptr; }

private:
    float m_ReverseMutProb;
};