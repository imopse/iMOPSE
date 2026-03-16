#pragma once

#include "method/operators/mutation/AMutation.h"

class CTTPReverseSingleFlip : public AMutation
{
public:
    explicit CTTPReverseSingleFlip(float reverseMutProb, float singleFlipMutProb) :
        m_ReverseMutProb(reverseMutProb),
        m_SingleFlipMutProb(singleFlipMutProb)
    {};
    ~CTTPReverseSingleFlip() override = default;

    void Mutate(SProblemEncoding& problemEncoding, AIndividual& child) override;

    size_t GetParamCount() const override { return 2; }
    float* GetParamValue(int paramIdx) override { return paramIdx == 0 ? &m_ReverseMutProb : (paramIdx == 1 ? &m_SingleFlipMutProb : nullptr); }

private:
    float m_ReverseMutProb;
    float m_SingleFlipMutProb;
};
