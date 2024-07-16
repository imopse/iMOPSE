#include "CECVRPTWRandomClientInsertion.h"

void CECVRPTWRandomClientInsertion::Mutate(SProblemEncoding& problemEncoding, AIndividual& child) {
	m_genotypeCopy->clear();
	m_missingCustomers->clear();

	auto& genotype = child.m_Genotype.m_IntGenotype;
	std::copy(genotype.begin(), genotype.end(), std::back_inserter(*m_genotypeCopy));
	std::sort(m_genotypeCopy->begin(), m_genotypeCopy->end());
	auto& problemTemplate = m_problem.GetECVRPTWTemplate();
	auto& cities = problemTemplate.GetCities();
	auto& distanceMatrix = problemTemplate.GetDistInfoMtx();
	auto& allCustomers = problemTemplate.GetCustomers();

	std::set_difference(allCustomers.begin()
		, allCustomers.end()
		, m_genotypeCopy->begin()
		, m_genotypeCopy->end()
		, std::inserter(*m_missingCustomers, m_missingCustomers->begin())
	);
	std::shuffle(m_missingCustomers->begin()
		, m_missingCustomers->end()
		, std::default_random_engine(std::chrono::system_clock::now().time_since_epoch().count())
	);

	for (int i = 0; i < m_missingCustomers->size(); i++) {
		int customerIdx = CRandom::GetInt(0, child.m_Genotype.m_IntGenotype.size() + 1);
		child.m_Genotype.m_IntGenotype.emplace(child.m_Genotype.m_IntGenotype.begin() + customerIdx, (*m_missingCustomers)[i]);
	}
}