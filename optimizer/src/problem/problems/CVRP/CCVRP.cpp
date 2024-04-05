#include "CCVRP.h"

#define TTP_SAVE_FIXED_GENES 0

CCVRP::CCVRP(CCVRPTemplate &cvrpBase) : m_CVRPTemplate(cvrpBase) {
    CreateProblemEncoding();

    m_MaxObjectiveValues = {
            m_CVRPTemplate.GetMaxDistance()
    };

    m_MinObjectiveValues = {
            m_CVRPTemplate.GetMinDistance()
    };
}

SProblemEncoding &CCVRP::GetProblemEncoding() {
    return m_ProblemEncoding;
}

size_t CCVRP::GetNearestDepotIdx(const size_t cityIdx) {
    float minDist = FLT_MAX;
    size_t chosenIdx;
    auto distMtx = m_CVRPTemplate.GetDistMtx();
    auto depotIndexes = m_CVRPTemplate.GetDepots();
    auto cities = m_CVRPTemplate.GetCities();

    for (const auto idx: depotIndexes) {
        int depot_index;
        for (int i =0;i<cities.size();i++){
            if (cities[i].m_ID==idx){
                depot_index=i;
                break;
            }
        }
        if (distMtx[cityIdx][depot_index] < minDist) {
            chosenIdx = depot_index;
            minDist = distMtx[cityIdx][depot_index];
        }
    }
    return chosenIdx;
}

void CCVRP::Evaluate(AIndividual& individual) {
    // Build solution
    auto &distMtx = m_CVRPTemplate.GetDistMtx();
    int capacity = m_CVRPTemplate.GetCapacity();

    std::vector<SCityCVRP> cities = m_CVRPTemplate.GetCities();
    std::vector<size_t> depotIndexes = m_CVRPTemplate.GetDepots();

    size_t citiesSize = m_CVRPTemplate.GetCitiesSize();

    // Evaluate
    int current_load = capacity;
    float distance = 0;
    for (size_t i = 0; i < citiesSize; ++i) {
        size_t cityIdx = individual.m_Genotype.m_IntGenotype[i];
        size_t nextCityIdx = individual.m_Genotype.m_IntGenotype[(i + 1) % citiesSize];

        if (current_load < cities[nextCityIdx].m_demand) {
            const size_t depotIdx = GetNearestDepotIdx(cityIdx);

            distance += distMtx[cityIdx][depotIdx];
            distance += distMtx[depotIdx][nextCityIdx];
            current_load = capacity;
        } else {
            distance += distMtx[cityIdx][nextCityIdx];
        }

        current_load -= cities[nextCityIdx].m_demand;
    }
    
    individual.m_Evaluation = {
            distance
    };
    
    // Normalize
    for (int i = 0; i < 1; i++)
    {
        individual.m_NormalizedEvaluation[i] = (individual.m_Evaluation[i] - m_MinObjectiveValues[i]) / (m_MaxObjectiveValues[i] - m_MinObjectiveValues[i]);
    }
}

void CCVRP::CreateProblemEncoding() {
    size_t citiesSize = m_CVRPTemplate.GetCitiesSize();

    SEncodingSection citiesSection = SEncodingSection
            {
                    // city indices <0, n-1>
                    std::vector<SEncodingDescriptor>(citiesSize, SEncodingDescriptor{
                            (float) 0, (float) (citiesSize - 1)
                    }),
                    EEncodingType::PERMUTATION
            };


    m_ProblemEncoding = SProblemEncoding{1, {citiesSection}, m_CVRPTemplate.GetDistMtx()};
}

void CCVRP::LogSolution(AIndividual& individual) {

}