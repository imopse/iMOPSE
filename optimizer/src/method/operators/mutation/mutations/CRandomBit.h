#pragma once

#include "../AMutation.h"
#include "../../../../problem/SProblemEncoding.h"

class CRandomBit : public AMutation
{
public:
    explicit CRandomBit(float mutationProbability) : m_MutationProbability(mutationProbability)
    {};
    ~CRandomBit() override = default;

    void Mutate(SProblemEncoding& problemEncoding, AIndividual &child) override;

    size_t GetParamCount() const override { return 1; }
    float* GetParamValue(int paramIdx) override { return paramIdx == 0 ? &m_MutationProbability : nullptr; }

private:
    float m_MutationProbability;
};
