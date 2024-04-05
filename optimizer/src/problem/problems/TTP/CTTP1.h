#pragma once

#include "CTTP2.h"

class CTTP1 : public CTTP2
{
public:
    explicit CTTP1(CTTP2 &ttp2);
    ~CTTP1() override = default;
    void Evaluate(AIndividual& individual) override;
};