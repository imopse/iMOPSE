#pragma once

#include "method/operators/mutation/AMutation.h"

class CTTPSwapFlip : public AMutation
{
public:
    explicit CTTPSwapFlip(float geneSwapProb, float flipMutProb) :
        m_GeneSwapProb(geneSwapProb),
        m_FlipMutProb(flipMutProb)
    {};
    ~CTTPSwapFlip() override = default;

    void Mutate(SProblemEncoding& problemEncoding, AIndividual& child) override;

    size_t GetParamCount() const override { return 2; }
    float* GetParamValue(int paramIdx) override { return paramIdx == 0 ? &m_GeneSwapProb : (paramIdx == 1 ? &m_FlipMutProb : nullptr); }

private:
    float m_GeneSwapProb;
    float m_FlipMutProb;
};
