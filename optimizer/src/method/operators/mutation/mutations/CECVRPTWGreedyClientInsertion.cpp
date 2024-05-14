#include "CECVRPTWGreedyClientInsertion.h"

void CECVRPTWGreedyClientInsertion::Mutate(SProblemEncoding& problemEncoding, AIndividual& child) {
	m_genotypeCopy->clear();
	m_missingCustomers->clear();

	m_problem.Evaluate(child);
	int intMax = INT32_MAX;;
	float evaluationScore = child.m_Evaluation[m_objectiveIndex];
	auto& genotype = child.m_Genotype.m_IntGenotype;
	std::copy(genotype.begin(), genotype.end(), std::back_inserter(*m_genotypeCopy));
	std::sort(m_genotypeCopy->begin(), m_genotypeCopy->end());
	auto& ecvrpTemplate = m_problem.GetECVRPTWTemplate();
	auto& allCustomers = ecvrpTemplate.GetCustomers();
	
	std::set_difference(allCustomers.begin()
		, allCustomers.end()
		, m_genotypeCopy->begin()
		, m_genotypeCopy->end()
		, std::inserter(*m_missingCustomers, m_missingCustomers->begin())
	);
	CRandom::Shuffle(0
		, m_missingCustomers->size()
		, *m_missingCustomers
	);

	for (int i = 0; i < m_missingCustomers->size(); i++) {
		size_t bestInsertLocation = -1;
		float bestCustomerScore = std::numeric_limits<float>::max();
		int randomPivot = CRandom::GetInt(0, genotype.size());
		int startPosition = CRandom::GetInt(std::max(0, randomPivot - GREEDYSIZE), randomPivot + 1);
		int endPosition = CRandom::GetInt(randomPivot + 1, std::min((int)genotype.size(), randomPivot + 1 + GREEDYSIZE - (randomPivot - startPosition)) + 1);
		for (int j = startPosition; j < endPosition + 1; j++) {
			genotype.insert(genotype.begin() + j, (*m_missingCustomers)[i]);
			m_problem.Evaluate(child);
			float newEvaluationScore = child.m_Evaluation[m_objectiveIndex];
			if (abs(evaluationScore - newEvaluationScore) < bestCustomerScore) {
				bestInsertLocation = j;
				bestCustomerScore = evaluationScore - newEvaluationScore;
			}
			genotype.erase(genotype.begin() + j);
		}
		genotype.insert(genotype.begin() + bestInsertLocation, (*m_missingCustomers)[i]);
		evaluationScore = bestCustomerScore;
	}
}