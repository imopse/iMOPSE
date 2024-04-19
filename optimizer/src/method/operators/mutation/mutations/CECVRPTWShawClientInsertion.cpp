#include "CECVRPTWShawClientInsertion.h"

void CECVRPTWShawClientInsertion::Mutate(SProblemEncoding& problemEncoding, AIndividual& child) {
	auto& genotype = child.m_Genotype.m_IntGenotype;
	auto genotypeCopy = std::vector<int>(genotype);
	std::sort(genotypeCopy.begin(), genotypeCopy.end());
	auto& problemTemplate = m_problem.GetECVRPTWTemplate();
	auto& cities = problemTemplate.GetCities();
	auto& distanceMatrix = problemTemplate.GetDistInfoMtx();
	auto& allCustomers = problemTemplate.GetCustomers();

	std::vector<size_t> missingCustomers;
	std::set_difference(allCustomers.begin()
		,  allCustomers.end()
		, genotypeCopy.begin()
		, genotypeCopy.end()
		, std::inserter(missingCustomers, missingCustomers.begin())
	);
	std::shuffle(missingCustomers.begin()
		, missingCustomers.end()
		, std::default_random_engine(std::chrono::system_clock::now().time_since_epoch().count())
	);

	for (int i = 0; i < missingCustomers.size(); i++) {
		auto& customerToCompare = cities[missingCustomers[i]];
		float minDistance = std::numeric_limits<float>::max();
		size_t customerWithMinDistanceIdx = -1;
		for (int j = 0; j < genotype.size(); j++) {
			if (genotype[j] != VEHICLE_DELIMITER) {
				float distanceDemand = DEMANDWEIGHT * abs(cities[genotype[j]].m_demand - customerToCompare.m_demand);
				float distanceTimeWindow = TIMEWINDOWWEIGHT * abs(cities[genotype[j]].m_readyTime - customerToCompare.m_readyTime);
				float distanceDistance = DISTANCEWEIGHT * distanceMatrix[missingCustomers[i]][genotype[j]].m_distance;
				if (distanceDemand + distanceTimeWindow + distanceDistance < minDistance) {
					customerWithMinDistanceIdx = j;
					minDistance = distanceDemand + distanceTimeWindow + distanceDistance;
				}
			}
		}
		genotype.insert(genotype.begin() + customerWithMinDistanceIdx, missingCustomers[i]);
	}
}