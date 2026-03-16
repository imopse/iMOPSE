#include "CCVRPConstructive.h"
#include "../individual/CGPHHIndividual.h"
#include "../../../../../problem/problems/CVRP/CCVRP.h"
#include <algorithm>
#include <limits>

void CCVRPConstructive::BuildSolution(CGPHHIndividual& individual, AProblem& problem) {
    CCVRP* cvrp = dynamic_cast<CCVRP*>(&problem);
    if (!cvrp) return;

    CCVRPTemplate& cvrpTemplate = cvrp->GetCVRPTemplate();
    const auto& cities = cvrpTemplate.GetCities();
    const auto& distMtx = cvrpTemplate.GetDistMtx();
    const auto& depots = cvrpTemplate.GetDepots();
    int capacity = cvrpTemplate.GetCapacity();

    individual.FlattenTree();

    std::vector<int> unvisited;
    unvisited.reserve(cities.size());
    std::vector<bool> isUnvisited(cities.size(), false);
    for (size_t i = 0; i < cities.size(); ++i) {
        bool isDepot = false;
        for (size_t d : depots) { if (cities[i].m_ID == d) { isDepot = true; break; } }
        if (!isDepot) { unvisited.push_back((int)i); isUnvisited[i] = true; }
    }

    individual.m_Genotype.m_IntGenotype.clear();
    int currentLoc = (int)cvrp->GetNearestDepotIdx(unvisited[0]);
    float currentLoad = 0;

    std::vector<float> nccCache(cities.size(), 0.0f);
    std::vector<size_t> nccPointer(cities.size(), 0);
    for (int idx : unvisited) {
        const auto& nearestNeighbors = cvrpTemplate.GetNearestNeighborsCache(idx);
        for (size_t i = 0; i < nearestNeighbors.size(); ++i) {
            if (isUnvisited[nearestNeighbors[i]]) {
                nccCache[idx] = distMtx[idx][nearestNeighbors[i]];
                nccPointer[idx] = i;
                break;
            }
        }
    }

    while (!unvisited.empty()) {
        int bestCandidate = unvisited[0];
        float bestScore = -std::numeric_limits<float>::max();

        for (int candidate : unvisited) {
            SGPHHContext context;
            context.features = { distMtx[currentLoc][candidate], (float)cities[candidate].m_demand, (float)(capacity - currentLoad), nccCache[candidate] };
            float score = individual.EvaluateIterative(context);
            if (score > bestScore) { bestScore = score; bestCandidate = candidate; }
        }

        if (currentLoad + cities[bestCandidate].m_demand > (float)capacity) currentLoad = 0;
        currentLoad += (float)cities[bestCandidate].m_demand;
        currentLoc = bestCandidate;

        isUnvisited[bestCandidate] = false;
        unvisited.erase(std::remove(unvisited.begin(), unvisited.end(), bestCandidate), unvisited.end());

        for (int idx : unvisited) {
            const auto& nearestNeighbors = cvrpTemplate.GetNearestNeighborsCache(idx);
            size_t& ptr = nccPointer[idx];
            if (ptr >= nearestNeighbors.size() || !isUnvisited[nearestNeighbors[ptr]]) {
                nccCache[idx] = 0.0f;
                for (size_t i = ptr + 1; i < nearestNeighbors.size(); ++i) {
                    if (isUnvisited[nearestNeighbors[i]]) { nccCache[idx] = distMtx[idx][nearestNeighbors[i]]; nccPointer[idx] = i; break; }
                }
            }
        }
        individual.m_Genotype.m_IntGenotype.push_back(bestCandidate);
    }
}
