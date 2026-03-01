#include "CMSRAClosestTask.h"
#include "utils/random/CRandom.h"
#include "problem/problems/MSRA/MSRAProblem.h"

CMSRAClosestTask::CMSRAClosestTask(float geneMutProb, const CMSRAProblem& problemDefinition)
    : m_GeneMutProb(geneMutProb)
    , m_ProblemDefinition(problemDefinition)
{}

void CMSRAClosestTask::Mutate(SProblemEncoding& problemEncoding, AIndividual& child)
{
    const size_t sectionSize = problemEncoding.m_Encoding[0].m_SectionDescription.size();
    for (size_t g = 0; g < sectionSize; ++g)
    {
        if (CRandom::GetFloat(0, 1) < m_GeneMutProb)
        {
            child.m_Genotype.m_FloatGenotype[g] = m_ProblemDefinition.FindBestGeneValueByClosestTask(child.m_Genotype.m_FloatGenotype, g);
        }
    }
}
