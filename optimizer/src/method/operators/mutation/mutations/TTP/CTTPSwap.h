#pragma once

#include "method/operators/mutation/AMutation.h"

class CTTPSwap : public AMutation
{
public:
    explicit CTTPSwap(float geneSwapProb) :
        m_GeneSwapProb(geneSwapProb)
    {};
    ~CTTPSwap() override = default;

    void Mutate(SProblemEncoding& problemEncoding, AIndividual& child) override;

    size_t GetParamCount() const override { return 1; }
    float* GetParamValue(int paramIdx) override { return paramIdx == 0 ? &m_GeneSwapProb : nullptr; }

private:
    float m_GeneSwapProb;
};
