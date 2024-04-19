#include "CECVRPTWGreedyClientInsertion.h"

void CECVRPTWGreedyClientInsertion::Mutate(SProblemEncoding& problemEncoding, AIndividual& child) {
	m_problem.Evaluate(child);
	int intMax = INT32_MAX;;
	float evaluationScore = m_problem.GetScore(child);
	auto& genotype = child.m_Genotype.m_IntGenotype;
	auto genotypeCopy = std::vector<int>(genotype);
	std::sort(genotypeCopy.begin(), genotypeCopy.end());
	auto& ecvrpTemplate = m_problem.GetECVRPTWTemplate();
	auto& allCustomers = ecvrpTemplate.GetCustomers();
	
	std::vector<int> missingCustomers;
	std::set_difference(allCustomers.begin()
		, allCustomers.end()
		, genotypeCopy.begin()
		, genotypeCopy.end()
		, std::inserter(missingCustomers, missingCustomers.begin())
	);
	CRandom::Shuffle(0
		, missingCustomers.size()
		, missingCustomers
	);

	for (int i = 0; i < missingCustomers.size(); i++) {
		size_t bestInsertLocation = -1;
		float bestCustomerScore = std::numeric_limits<float>::max();
		for (int j = 0; j < genotype.size(); j++) {
			genotype.insert(genotype.begin() + j, missingCustomers[i]);
			m_problem.Evaluate(child);
			float newEvaluationScore = m_problem.GetScore(child);
			if (abs(evaluationScore - newEvaluationScore) < bestCustomerScore) {
				bestInsertLocation = j;
				bestCustomerScore = evaluationScore - newEvaluationScore;
			}
			genotype.erase(genotype.begin() + j);
		}
		genotype.insert(genotype.begin() + bestInsertLocation, missingCustomers[i]);
		evaluationScore = bestCustomerScore;
	}
}