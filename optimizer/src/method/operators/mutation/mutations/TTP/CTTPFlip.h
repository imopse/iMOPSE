#pragma once

#include "method/operators/mutation/AMutation.h"

class CTTPFlip : public AMutation
{
public:
    explicit CTTPFlip(float flipMutProb) :
        m_FlipMutProb(flipMutProb)
    {};
    ~CTTPFlip() override = default;

    void Mutate(SProblemEncoding& problemEncoding, AIndividual& child) override;

    size_t GetParamCount() const override { return 1; }
    float* GetParamValue(int paramIdx) override { return paramIdx == 0 ? &m_FlipMutProb : nullptr; }

private:
    float m_FlipMutProb;
};
