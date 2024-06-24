#include "CECVRPTWShawClientInsertion.h"

void CECVRPTWShawClientInsertion::Mutate(SProblemEncoding& problemEncoding, AIndividual& child) {
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
		,  allCustomers.end()
		, m_genotypeCopy->begin()
		, m_genotypeCopy->end()
		, std::inserter(*m_missingCustomers, m_missingCustomers->begin())
	);
	std::shuffle(m_missingCustomers->begin()
		, m_missingCustomers->end()
		, std::default_random_engine(std::chrono::system_clock::now().time_since_epoch().count())
	);

	for (int i = 0; i < m_missingCustomers->size(); i++) {
		auto& customerToCompare = cities[(*m_missingCustomers)[i]];
		float minDistance = std::numeric_limits<float>::max();
		size_t customerWithMinDistanceIdx = -1;
		for (int j = 0; j < genotype.size(); j++) {
			if (genotype[j] != VEHICLE_DELIMITER) {
				float distanceDemand = DEMANDWEIGHT * abs(cities[genotype[j]].m_demand - customerToCompare.m_demand);
				float distanceTimeWindow = TIMEWINDOWWEIGHT * abs(cities[genotype[j]].m_readyTime - customerToCompare.m_readyTime);
				float distanceDistance = DISTANCEWEIGHT * distanceMatrix[(*m_missingCustomers)[i]][genotype[j]].m_distance;
				if (distanceDemand + distanceTimeWindow + distanceDistance < minDistance) {
					customerWithMinDistanceIdx = j;
					minDistance = distanceDemand + distanceTimeWindow + distanceDistance;
				}
			}
		}
		genotype.insert(genotype.begin() + customerWithMinDistanceIdx, (*m_missingCustomers)[i]);
	}
}