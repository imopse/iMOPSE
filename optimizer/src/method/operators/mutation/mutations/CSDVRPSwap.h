#pragma once
#include "method/operators/mutation/AMutation.h"

class CSDVRPSwap : public AMutation {
public:
    explicit CSDVRPSwap(float mutationProbability) : m_mutationProbability(mutationProbability) {
    };

    ~CSDVRPSwap() override = default;

    void Mutate(SProblemEncoding &problemEncoding, AIndividual &child) override;

private:
    float m_mutationProbability;
};
