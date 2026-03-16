#pragma once

#include "method/operators/mutation/AMutation.h"

class CTTP2;

class CTTPRandomSingleOpt2 : public AMutation
{
public:
    explicit CTTPRandomSingleOpt2(const CTTP2& problemDefinition) :
        m_ProblemDefinition(problemDefinition)
    {};
    ~CTTPRandomSingleOpt2() override = default;

    void Mutate(SProblemEncoding& problemEncoding, AIndividual& child) override;

    size_t GetParamCount() const override { return 0; }
    float* GetParamValue(int paramIdx) override { return nullptr; }

private:
    const CTTP2& m_ProblemDefinition;
};
