#include "CCVRP.h"
#include <iostream>
#include <algorithm>
#include <limits>

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
    size_t chosenIdx = 0;
    auto& distMtx = m_CVRPTemplate.GetDistMtx();
    auto& depotIndexes = m_CVRPTemplate.GetDepots();
    auto& cities = m_CVRPTemplate.GetCities();

    for (const auto idx : depotIndexes) {
        int depot_index = -1;
        for (int i = 0; i < (int)cities.size(); i++) {
            if (cities[i].m_ID == idx) {
                depot_index = i;
                break;
            }
        }
        if (depot_index != -1 && distMtx[cityIdx][depot_index] < minDist) {
            chosenIdx = depot_index;
            minDist = distMtx[cityIdx][depot_index];
        }
    }
    return chosenIdx;
}
void CCVRP::Evaluate(AIndividual& individual) {
    if (individual.m_Genotype.m_IntGenotype.empty()) return;

    auto& distMtx = m_CVRPTemplate.GetDistMtx();
    int capacity = m_CVRPTemplate.GetCapacity();
    const std::vector<SCityCVRP>& cities = m_CVRPTemplate.GetCities();
    int current_load = capacity;
    current_load -= cities[individual.m_Genotype.m_IntGenotype[0]].m_demand;
    float distance = distMtx[GetNearestDepotIdx(individual.m_Genotype.m_IntGenotype[0])][individual.m_Genotype.m_IntGenotype[0]];
    size_t lastCityIdx = individual.m_Genotype.m_IntGenotype[0];

    for (size_t i = 0; i < individual.m_Genotype.m_IntGenotype.size() - 1; ++i) {
        size_t cityIdx = individual.m_Genotype.m_IntGenotype[i];
        size_t nextCityIdx = individual.m_Genotype.m_IntGenotype[i+1];
        if (current_load < cities[nextCityIdx].m_demand) {
            distance += distMtx[cityIdx][GetNearestDepotIdx(cityIdx)] + distMtx[GetNearestDepotIdx(cityIdx)][nextCityIdx];
            current_load = capacity;
        } else {
            distance += distMtx[cityIdx][nextCityIdx];
        }
        current_load -= cities[nextCityIdx].m_demand;
        lastCityIdx = nextCityIdx;
    }
    distance += distMtx[lastCityIdx][GetNearestDepotIdx(lastCityIdx)];
    individual.m_Evaluation[0] = distance;
    individual.m_NormalizedEvaluation[0] = distance / GetOptimalValue();
}

float CCVRP::GetOptimalValue() { return m_CVRPTemplate.GetOptimalValue(); }

void CCVRP::CreateProblemEncoding() {
    size_t citiesSize = m_CVRPTemplate.GetCitiesSize();
    SEncodingSection citiesSection = { std::vector<SEncodingDescriptor>(citiesSize, SEncodingDescriptor{0.0f, (float)(citiesSize - 1)}), EEncodingType::PERMUTATION };
    m_ProblemEncoding = SProblemEncoding{ 1, {citiesSection}, m_CVRPTemplate.GetDistMtx() };
}

void CCVRP::LogSolution(AIndividual& individual) {}