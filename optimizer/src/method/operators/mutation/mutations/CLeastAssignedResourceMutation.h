#pragma once

#include "../AMutation.h"

class CMSRCPSP_TA;

class CLeastAssignedResourceMutation : public AMutation
{
public:
    CLeastAssignedResourceMutation(float geneMutProb, const CMSRCPSP_TA& problemDefinition);
    void Mutate(SProblemEncoding& problemEncoding, AIndividual& child) override;

private:
    float m_GeneMutProb;
    const CMSRCPSP_TA& m_ProblemDefinition;
};

