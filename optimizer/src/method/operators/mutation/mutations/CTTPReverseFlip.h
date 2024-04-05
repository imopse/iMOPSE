#pragma once

#include "../AMutation.h"

class CTTPReverseFlip : public AMutation
{
public:
    explicit CTTPReverseFlip(float reverseMutProb, float flipMutProb) : m_ReverseMutProb(reverseMutProb),
                                                                        m_FlipMutProb(flipMutProb)
    {};
    ~CTTPReverseFlip() override = default;

    void Mutate(SProblemEncoding& problemEncoding, AIndividual &child) override;
private:
    float m_ReverseMutProb;
    float m_FlipMutProb;
};
