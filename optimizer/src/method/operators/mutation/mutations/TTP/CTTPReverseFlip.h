#pragma once

#include "method/operators/mutation/AMutation.h"

class CTTPReverseFlip : public AMutation
{
public:
    explicit CTTPReverseFlip(float reverseMutProb, float flipMutProb) : m_ReverseMutProb(reverseMutProb),
                                                                        m_FlipMutProb(flipMutProb)
    {};
    ~CTTPReverseFlip() override = default;

    void Mutate(SProblemEncoding& problemEncoding, AIndividual& child) override;

    size_t GetParamCount() const override { return 2; }
    float* GetParamValue(int paramIdx) override { return paramIdx == 0 ? &m_ReverseMutProb : (paramIdx == 1 ? &m_FlipMutProb : nullptr); }

private:
    float m_ReverseMutProb;
    float m_FlipMutProb;
};
