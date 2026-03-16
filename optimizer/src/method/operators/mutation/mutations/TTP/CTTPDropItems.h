#pragma once

#include "method/operators/mutation/AMutation.h"

class CTTPDropItems : public AMutation
{
public:
    explicit CTTPDropItems(float dropItemProb) :
        m_DropItemProb(dropItemProb)
    {};
    ~CTTPDropItems() override = default;

    void Mutate(SProblemEncoding& problemEncoding, AIndividual& child) override;

    size_t GetParamCount() const override { return 1; }
    float* GetParamValue(int paramIdx) override { return paramIdx == 0 ? &m_DropItemProb : nullptr; }

private:
    float m_DropItemProb;
};

