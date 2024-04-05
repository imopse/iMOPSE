#include "CLeastAssignedResourceMutation.h"
#include "utils/random/CRandom.h"
#include "problem/problems/MSRCPSP/CMSRCPSP_TA.h"

CLeastAssignedResourceMutation::CLeastAssignedResourceMutation(float geneMutProb, const CMSRCPSP_TA& problemDefinition)
    : m_GeneMutProb(geneMutProb)
    , m_ProblemDefinition(problemDefinition)
{}

void CLeastAssignedResourceMutation::Mutate(SProblemEncoding& problemEncoding, AIndividual& child)
{
    // TODO - it seems MSRCPSP uses FloatGenotype and this operator is dedicated for MSRCPSP but we should have one genotype value type
    std::vector<size_t> resourcesUsage = m_ProblemDefinition.FindNumberOfResourcesUse(child.m_Genotype.m_FloatGenotype);
    const size_t sectionSize = problemEncoding.m_Encoding[0].m_SectionDescription.size();
    for (size_t g = 0; g < sectionSize; ++g)
    {
        if (CRandom::GetFloat(0, 1) < m_GeneMutProb)
        {
            // We do not recalculate the usage after each gene to keep it simple
            child.m_Genotype.m_FloatGenotype[g] = m_ProblemDefinition.FindBestGeneValueUsageWise(g, resourcesUsage);
        }
    }
}
