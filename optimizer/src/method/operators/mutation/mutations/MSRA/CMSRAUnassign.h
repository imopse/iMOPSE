#pragma once

#include "method/operators/mutation/AMutation.h"

class CMSRAProblem;

class CMSRAUnassign : public AMutation
{
public:
    CMSRAUnassign(float geneMutProb, const CMSRAProblem& problemDefinition);
    void Mutate(SProblemEncoding& problemEncoding, AIndividual& child) override;

    size_t GetParamCount() const override { return 1; }
    float* GetParamValue(int paramIdx) override { return paramIdx == 0 ? &m_GeneMutProb : nullptr; }


private:
    float m_GeneMutProb;
    const CMSRAProblem& m_ProblemDefinition;
};

