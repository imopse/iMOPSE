#pragma once

#include "method/operators/mutation/AMutation.h"

class CTTP2;

class CTTPForceLastItems : public AMutation
{
public:
    explicit CTTPForceLastItems(const CTTP2& problemDefinition) :
        m_ProblemDefinition(problemDefinition)
    {};
    ~CTTPForceLastItems() override = default;

    void Mutate(SProblemEncoding& problemEncoding, AIndividual& child) override;

    size_t GetParamCount() const override { return 0; }
    float* GetParamValue(int paramIdx) override { return nullptr; }

private:
    const CTTP2& m_ProblemDefinition;
};
