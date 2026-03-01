#pragma once

#include "method/operators/mutation/AMutation.h"

class CTTPSingleFlip : public AMutation
{
public:
    explicit CTTPSingleFlip(float singleFlipMutProb) :
        m_SingleFlipMutProb(singleFlipMutProb)
    {};
    ~CTTPSingleFlip() override = default;

    void Mutate(SProblemEncoding& problemEncoding, AIndividual& child) override;

    size_t GetParamCount() const override { return 1; }
    float* GetParamValue(int paramIdx) override { return paramIdx == 0 ? &m_SingleFlipMutProb : nullptr; }

private:
    float m_SingleFlipMutProb;
};
