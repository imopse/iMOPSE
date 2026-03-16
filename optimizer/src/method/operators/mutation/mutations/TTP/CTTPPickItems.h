#pragma once

#include "method/operators/mutation/AMutation.h"

class CTTPPickItems : public AMutation
{
public:
    explicit CTTPPickItems(float pickItemProb) :
        m_PickItemProb(pickItemProb)
    {};
    ~CTTPPickItems() override = default;

    void Mutate(SProblemEncoding& problemEncoding, AIndividual& child) override;

    size_t GetParamCount() const override { return 1; }
    float* GetParamValue(int paramIdx) override { return paramIdx == 0 ? &m_PickItemProb : nullptr; }

private:
    float m_PickItemProb;
};
