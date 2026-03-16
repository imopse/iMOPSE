#pragma once

#include "../AMutation.h"

class CMSRCPSP_TA;

class CCheapestResourceMutation : public AMutation
{
public:
    CCheapestResourceMutation(float geneMutProb, const CMSRCPSP_TA& problemDefinition);
    void Mutate(SProblemEncoding& problemEncoding, AIndividual& child) override;

    size_t GetParamCount() const override { return 1; }
    float* GetParamValue(int paramIdx) override { return paramIdx == 0 ? &m_GeneMutProb : nullptr; }

private:
    float m_GeneMutProb;
    const CMSRCPSP_TA& m_ProblemDefinition;
};
