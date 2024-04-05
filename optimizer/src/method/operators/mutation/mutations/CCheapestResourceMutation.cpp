#include "CCheapestResourceMutation.h"
#include "utils/random/CRandom.h"
#include "problem/problems/MSRCPSP/CMSRCPSP_TA.h"

CCheapestResourceMutation::CCheapestResourceMutation(float geneMutProb, const CMSRCPSP_TA& problemDefinition)
    : m_GeneMutProb(geneMutProb)
    , m_ProblemDefinition(problemDefinition)
{}

void CCheapestResourceMutation::Mutate(SProblemEncoding& problemEncoding, AIndividual& child)
{
    // TODO - it seems MSRCPSP uses FloatGenotype and this operator is dedicated for MSRCPSP but we should have one genotype value type
    const size_t sectionSize = problemEncoding.m_Encoding[0].m_SectionDescription.size();
    for (size_t g = 0; g < sectionSize; ++g)
    {
        if (CRandom::GetFloat(0, 1) < m_GeneMutProb)
        {
            child.m_Genotype.m_FloatGenotype[g] = m_ProblemDefinition.FindBestGeneValueCostWise(g);
        }
    }
}
