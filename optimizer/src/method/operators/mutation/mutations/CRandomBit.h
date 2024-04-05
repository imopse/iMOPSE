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
private:
    float m_MutationProbability;
};
