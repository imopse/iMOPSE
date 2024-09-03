#include "CECVRPTWRandomClientInsertion.h"
#include "problem/problems/ECVRPTW/CECVRPTW.h"
#include "utils/random/CRandom.h"

CECVRPTWRandomClientInsertion::CECVRPTWRandomClientInsertion(CECVRPTW& problemDefinition)
    : m_ProblemDefinition(problemDefinition)
{}

void CECVRPTWRandomClientInsertion::Mutate(SProblemEncoding& problemEncoding, AIndividual& child)
{
	auto& genotype = child.m_Genotype.m_IntGenotype;
    auto genotypeCopy = genotype;
	std::sort(genotypeCopy.begin(), genotypeCopy.end());
	auto& problemTemplate = m_ProblemDefinition.GetECVRPTWTemplate();
	auto& allCustomers = problemTemplate.GetCustomers();

    std::vector<int> missingCustomers;
    std::set_difference(allCustomers.begin(), allCustomers.end(),
                        genotypeCopy.begin(), genotypeCopy.end(),
                        std::inserter(missingCustomers, missingCustomers.begin()));
    CRandom::Shuffle(0, missingCustomers.size(), missingCustomers);

	for (int i = 0; i < missingCustomers.size(); i++)
    {
		int customerIdx = CRandom::GetInt(0, child.m_Genotype.m_IntGenotype.size() + 1);
		child.m_Genotype.m_IntGenotype.emplace(child.m_Genotype.m_IntGenotype.begin() + customerIdx, missingCustomers[i]);
	}
}