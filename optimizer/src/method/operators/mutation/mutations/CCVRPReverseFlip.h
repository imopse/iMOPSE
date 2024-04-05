//
#pragma once

#include "../AMutation.h"

class CCVRPReverseFlip : public AMutation
{
public:
    explicit CCVRPReverseFlip(float reverseMutProb) : m_ReverseMutProb(reverseMutProb)
    {};
    ~CCVRPReverseFlip() override = default;

    void Mutate(SProblemEncoding& problemEncoding, AIndividual &child) override;
private:
    float m_ReverseMutProb;
};