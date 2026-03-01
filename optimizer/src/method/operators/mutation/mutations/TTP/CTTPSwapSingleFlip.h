#pragma once

#include "method/operators/mutation/AMutation.h"

class CTTPSwapSingleFlip : public AMutation
{
public:
    explicit CTTPSwapSingleFlip(float geneSwapProb, float singleFlipMutProb) :
        m_GeneSwapProb(geneSwapProb),
        m_SingleFlipMutProb(singleFlipMutProb)
    {};
    ~CTTPSwapSingleFlip() override = default;

    void Mutate(SProblemEncoding& problemEncoding, AIndividual& child) override;

    size_t GetParamCount() const override { return 2; }
    float* GetParamValue(int paramIdx) override { return paramIdx == 0 ? &m_GeneSwapProb : (paramIdx == 1 ? &m_SingleFlipMutProb : nullptr); }

private:
    float m_GeneSwapProb;
    float m_SingleFlipMutProb;
};
