#include "CSDVRP.h"

#include <iostream>
#include <algorithm>

CSDVRP::CSDVRP(CSDVRPTemplate &sdvrpBase) : m_SDVRPTemplate(sdvrpBase) {
    CreateProblemEncoding();

    m_MaxObjectiveValues = {
            m_SDVRPTemplate.GetMaxDistance()
    };

    m_MinObjectiveValues = {
            m_SDVRPTemplate.GetMinDistance()
    };
}

SProblemEncoding &CSDVRP::GetProblemEncoding() {
    return m_ProblemEncoding;
}

void CSDVRP::Evaluate(AIndividual &individual) {

    const std::vector<std::vector<int> > distanceMtx = m_SDVRPTemplate.GetDistMtx();
    const int capacity = m_SDVRPTemplate.GetCapacity();
    constexpr int depot = 0;

    std::vector<int> demands(m_SDVRPTemplate.GetCitiesSize());

    for (int i = 0; i < m_SDVRPTemplate.GetCitiesSize(); ++i) {
        demands[i] = m_SDVRPTemplate.GetCities()[i].m_demand;
    }

    const std::vector<int> genome = individual.m_Genotype.m_IntGenotype;

    int currentLoad = capacity;
    int distance = 0;

    distance += distanceMtx[depot][genome[0] + 1];

    int currentCityIdx = 0;
    bool isDepot = false;

    while (true) {
        if (isDepot) {
            distance = distance + distanceMtx[genome[currentCityIdx] + 1][depot];
            currentLoad = capacity;

            int closestCityIdx = -1;
            int closestCityDist = INT_MAX;

            for (int i = 0; i < demands.size(); ++i) {
                if (demands[genome[i]] < m_SDVRPTemplate.GetCities()[genome[i]].m_demand && demands[genome[i]] > 0) {
                    int dist = distanceMtx[depot][genome[i] + 1];
                    if (dist < closestCityDist) {
                        closestCityIdx = i;
                        closestCityDist = dist;
                    }
                }
            }
            if (closestCityIdx == -1) {
                closestCityIdx = currentCityIdx + 1;
            }
            distance = distance + distanceMtx[depot][genome[closestCityIdx] + 1];
            isDepot = false;
            currentCityIdx = closestCityIdx;
        }

        if (currentLoad >= demands[genome[currentCityIdx]]) {
            currentLoad -= demands[genome[currentCityIdx]];
            demands[genome[currentCityIdx]] = 0;
            if (currentCityIdx == genome.size() - 1) {
                distance += distanceMtx[genome[currentCityIdx] + 1][depot];
                break;
            }
            if (currentLoad > 0) {
                int closestCityIdx = -1;
                int closestCityDist = INT_MAX;
                for (int i = 0; i < demands.size(); ++i) {
                    if (demands[genome[i]] < m_SDVRPTemplate.GetCities()[genome[i]].m_demand && demands[genome[i]] >
                                                                                                0) {
                        int dist = distanceMtx[genome[currentCityIdx] + 1][genome[i] + 1];
                        if (dist < closestCityDist) {
                            closestCityIdx = i;
                            closestCityDist = dist;
                        }
                    }
                }
                if (closestCityIdx == -1) {
                    closestCityIdx = currentCityIdx + 1;
                }
                distance = distance + distanceMtx[genome[currentCityIdx] + 1][genome[closestCityIdx] + 1];
                currentCityIdx = closestCityIdx;
            }
            else {
                isDepot = true;
            }
        } else {
            demands[genome[currentCityIdx]] -= currentLoad;
            currentLoad = 0;
            isDepot = true;
        }
    }

    individual.m_Evaluation = {
            static_cast<float>(distance)
    };

    for (int i = 0; i < 1; i++) {
        individual.m_NormalizedEvaluation[i] = (individual.m_Evaluation[i] - m_MinObjectiveValues[i]) /
                                               (m_MaxObjectiveValues[i] - m_MinObjectiveValues[i]);
    }
}

void CSDVRP::LogSolution(AIndividual &individual) {
    const auto eval = individual.m_Evaluation;
    std::cout << "==================================================================" << std::endl;
    if (!eval.empty()) {
        std::cout << "Solution: " << static_cast<int>(eval.front()) << std::endl;
    } else {
        std::cout << "Solution empty" << std::endl;
    }

    const std::vector<int> genotype = individual.m_Genotype.m_IntGenotype;
    std::cout << "Genome: " << "(genomeSize: " << genotype.size() << ")" << std::endl;
    std::cout << "[ ";
    const int lastIdx = genotype.size() - 1;
    for (int i = 0; i < genotype.size(); ++i) {
        if (i != lastIdx) {
            std::cout << genotype[i] + 1 << ", ";
        } else {
            std::cout << genotype[i] + 1 << " ]" << std::endl;
        }
    }

    const std::vector<int> realPath = EvaluateAndBuildRealPath(individual);
    std::cout << "Route: " << "(routeSize: " << realPath.size() << ")" << std::endl;
    std::cout << "[ ";
    const int lastCityIdx = realPath.size() - 1;
    for (int i = 0; i < realPath.size(); ++i) {
        if (i != lastCityIdx) {
            std::cout << realPath[i] << ", ";
        } else {
            std::cout << realPath[i] << " ]" << std::endl;
        }
    }
}

size_t CSDVRP::GetNearestDepotIdx(size_t cityIdx) const {
    auto depot = m_SDVRPTemplate.GetDepot();
    return depot.m_ID;
}

void CSDVRP::CreateProblemEncoding() {
    const size_t citiesSize = m_SDVRPTemplate.GetCitiesSize();

    SEncodingSection citiesSection = SEncodingSection
            {
                    // city indices <0, n-1>
                    std::vector(citiesSize, SEncodingDescriptor{
                            (float) 0, (float) (citiesSize - 1)
                    }),
                    EEncodingType::PERMUTATION
            };

    std::vector<std::vector<int> > distMtx = m_SDVRPTemplate.GetDistMtx();
    std::vector<std::vector<float> > castDistMtx;
    castDistMtx.reserve(distMtx.size());
    for (const auto &row: distMtx) {
        castDistMtx.emplace_back(row.begin(), row.end());
    }
    m_ProblemEncoding = SProblemEncoding{1, {citiesSection}, castDistMtx};
}