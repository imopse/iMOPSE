#include "CECVRPTWShawClientRemoval.h"

void CECVRPTWShawClientRemoval::Mutate(SProblemEncoding& problemEncoding, AIndividual& child) {
	int customersToRemove = CRandom::GetInt(1, problemEncoding.m_Encoding[0].m_SectionDescription.size() - m_problem.GetECVRPTWTemplate().GetVehicleCount() - 1);
	std::vector<int> customerIndexes;
	customerIndexes.reserve(customersToRemove);
	int firstCustomerIdx = CRandom::GetInt(1, problemEncoding.m_Encoding[0].m_SectionDescription.size());
	auto& genotype = child.m_Genotype.m_IntGenotype;
	while (child.m_Genotype.m_IntGenotype[firstCustomerIdx] == VEHICLE_DELIMITER) {
		firstCustomerIdx = CRandom::GetInt(1, problemEncoding.m_Encoding[0].m_SectionDescription.size());
	}

	auto& problemTemplate = m_problem.GetECVRPTWTemplate();
	auto& cities = problemTemplate.GetCities();
	auto& distanceMatrix = problemTemplate.GetDistInfoMtx();

	customerIndexes.push_back(genotype[firstCustomerIdx]);

	genotype.erase(genotype.begin() + firstCustomerIdx);
	for (int i = 1; i < customersToRemove; i++) {
		size_t customerToCompareIdx = CRandom::GetInt(0, customerIndexes.size());
		auto& customerToCompare = cities[customerIndexes[customerToCompareIdx]];
		float minDistance = std::numeric_limits<float>::max();
		size_t customerWithMinDistanceIdx = -1;
		for (int j = 0; j < genotype.size(); j++) {
			if (genotype[j] != VEHICLE_DELIMITER) {
				float distanceDemand = DEMANDWEIGHT * abs(cities[genotype[j]].m_demand - customerToCompare.m_demand);
				float distanceTimeWindow = TIMEWINDOWWEIGHT * abs(cities[genotype[j]].m_readyTime - customerToCompare.m_readyTime);
				float distanceDistance = DISTANCEWEIGHT * distanceMatrix[customerToCompareIdx][genotype[j]].m_distance;
				if (distanceDemand + distanceTimeWindow + distanceDistance < minDistance) {
					customerWithMinDistanceIdx = j;
					minDistance = distanceDemand + distanceTimeWindow + distanceDistance;
				}
			}
		}
		customerIndexes.push_back(genotype[customerWithMinDistanceIdx]);
		genotype.erase(genotype.begin() + customerWithMinDistanceIdx);
	}
}