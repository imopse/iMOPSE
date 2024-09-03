#include "CECVRPTWShawClientInsertion.h"
#include "problem/problems/ECVRPTW/CECVRPTW.h"
#include "utils/random/CRandom.h"

#define DEMANDWEIGHT 0.1
#define TIMEWINDOWWEIGHT 0.6
#define DISTANCEWEIGHT 0.3

CECVRPTWShawClientInsertion::CECVRPTWShawClientInsertion(CECVRPTW& problemDefinition)
    : m_ProblemDefinition(problemDefinition)
{}

void CECVRPTWShawClientInsertion::Mutate(SProblemEncoding& problemEncoding, AIndividual& child)
{
	auto& genotype = child.m_Genotype.m_IntGenotype;
    auto genotypeCopy = genotype;
	std::sort(genotypeCopy.begin(), genotypeCopy.end());
	auto& problemTemplate = m_ProblemDefinition.GetECVRPTWTemplate();
	auto& cities = problemTemplate.GetCities();
	auto& distanceMatrix = problemTemplate.GetDistInfoMtx();
	auto& allCustomers = problemTemplate.GetCustomers();

    std::vector<int> missingCustomers;
    std::set_difference(allCustomers.begin(), allCustomers.end(),
                        genotypeCopy.begin(), genotypeCopy.end(),
                        std::inserter(missingCustomers, missingCustomers.begin()));
    CRandom::Shuffle(0, missingCustomers.size(), missingCustomers);

	for (int i = 0; i < missingCustomers.size(); i++)
    {
		auto& customerToCompare = cities[missingCustomers[i]];
		float minDistance = std::numeric_limits<float>::max();
		size_t customerWithMinDistanceIdx = -1;
		for (int j = 0; j < genotype.size(); j++)
        {
			if (genotype[j] != VEHICLE_DELIMITER)
            {
				float distanceDemand = DEMANDWEIGHT * abs(cities[genotype[j]].m_Demand - customerToCompare.m_Demand);
				float distanceTimeWindow = TIMEWINDOWWEIGHT * abs(cities[genotype[j]].m_ReadyTime - customerToCompare.m_ReadyTime);
				float distanceDistance = DISTANCEWEIGHT * distanceMatrix[missingCustomers[i]][genotype[j]].m_Distance;
				if (distanceDemand + distanceTimeWindow + distanceDistance < minDistance)
                {
					customerWithMinDistanceIdx = j;
					minDistance = distanceDemand + distanceTimeWindow + distanceDistance;
				}
			}
		}
		genotype.insert(genotype.begin() + customerWithMinDistanceIdx, missingCustomers[i]);
	}
}