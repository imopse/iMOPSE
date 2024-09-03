#include "CECVRPTWShawClientRemoval.h"
#include "problem/problems/ECVRPTW/CECVRPTW.h"
#include "utils/random/CRandom.h"

#define DEMANDWEIGHT 0.1
#define TIMEWINDOWWEIGHT 0.6
#define DISTANCEWEIGHT 0.3

CECVRPTWShawClientRemoval::CECVRPTWShawClientRemoval(CECVRPTW& problemDefinition)
    : m_ProblemDefinition(problemDefinition)
{
    // TODO - verify whether "vehicle count - 1" should be in parenthesis
    size_t indicesCount = m_ProblemDefinition.GetProblemEncoding().m_Encoding[0].m_SectionDescription.size() - m_ProblemDefinition.GetECVRPTWTemplate().GetVehicleCount() - 1;
    m_CustomerIndexes.reserve(indicesCount);
}

void CECVRPTWShawClientRemoval::Mutate(SProblemEncoding& problemEncoding, AIndividual& child) {
    m_CustomerIndexes.clear();

    // TODO - verify whether "vehicle count - 1" should be in parenthesis
	int customersToRemove = CRandom::GetInt(1, problemEncoding.m_Encoding[0].m_SectionDescription.size() - m_ProblemDefinition.GetECVRPTWTemplate().GetVehicleCount() - 1);
	int firstCustomerIdx = CRandom::GetInt(1, problemEncoding.m_Encoding[0].m_SectionDescription.size());
	auto& genotype = child.m_Genotype.m_IntGenotype;
	while (child.m_Genotype.m_IntGenotype[firstCustomerIdx] == VEHICLE_DELIMITER) {
		firstCustomerIdx = CRandom::GetInt(1, problemEncoding.m_Encoding[0].m_SectionDescription.size());
	}

	auto& problemTemplate = m_ProblemDefinition.GetECVRPTWTemplate();
	auto& cities = problemTemplate.GetCities();
	auto& distanceMatrix = problemTemplate.GetDistInfoMtx();

    m_CustomerIndexes.push_back(genotype[firstCustomerIdx]);

	genotype.erase(genotype.begin() + firstCustomerIdx);
	for (int i = 1; i < customersToRemove; i++) {
		size_t customerToCompareIdx = CRandom::GetInt(0, m_CustomerIndexes.size());
		auto& customerToCompare = cities[m_CustomerIndexes[customerToCompareIdx]];
		float minDistance = std::numeric_limits<float>::max();
		size_t customerWithMinDistanceIdx = -1;
		for (int j = 0; j < genotype.size(); j++) {
			if (genotype[j] != VEHICLE_DELIMITER) {
				float distanceDemand = DEMANDWEIGHT * abs(cities[genotype[j]].m_Demand - customerToCompare.m_Demand);
				float distanceTimeWindow = TIMEWINDOWWEIGHT * abs(cities[genotype[j]].m_ReadyTime - customerToCompare.m_ReadyTime);
				float distanceDistance = DISTANCEWEIGHT * distanceMatrix[customerToCompareIdx][genotype[j]].m_Distance;
				if (distanceDemand + distanceTimeWindow + distanceDistance < minDistance) {
					customerWithMinDistanceIdx = j;
					minDistance = distanceDemand + distanceTimeWindow + distanceDistance;
				}
			}
		}
        m_CustomerIndexes.push_back(genotype[customerWithMinDistanceIdx]);
		genotype.erase(genotype.begin() + customerWithMinDistanceIdx);
	}
}
