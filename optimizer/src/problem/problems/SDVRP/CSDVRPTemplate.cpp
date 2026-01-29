#include "CSDVRPTemplate.h"

SCitySDVRP::SCitySDVRP(const int &id, const int &index, const float &x, const float &y, const int &demand) {
    m_ID = id;
    m_index = index;
    m_PosX = x;
    m_PosY = y;
    m_demand = demand;
}

void CSDVRPTemplate::Clear() {
    m_Cities.clear();
    m_Capacity = 0;
    m_Trucks = 0;

    m_DistanceMatrix.clear();
    m_MinDistanceVec.clear();
}

void CSDVRPTemplate::SetData(const std::vector<SCitySDVRP> &cities, int capacity, int trucks,
                             const SCitySDVRP &depot) {
    Clear();

    m_Cities = cities;
    m_Capacity = capacity;
    m_Trucks = trucks;
    m_Depot = depot;

    CalculateContextData();
}

int CSDVRPTemplate::GetMinDistance() const {
    int dist = 0;
    size_t dim = m_DistanceMatrix.size();
    for (size_t i = 0; i < dim; ++i) {
        int minDist = INT_MAX;
        for (size_t j = 0; j < dim; ++j) {
            if (i != j) {
                minDist = std::min(minDist, m_DistanceMatrix[i][j]);
            }
        }
        dist += minDist;
    }
    return dist;
}

int CSDVRPTemplate::GetMaxDistance() const {
    // maximum distance is twice the minimum distance
    return GetMinDistance() * 2;
}

void CSDVRPTemplate::CalculateContextData() {
    // Calculate distance matrix
    auto depot = m_Depot;

    size_t dim = m_Cities.size() + 1;

    std::vector<SCitySDVRP> citiesWithDepot(dim);
    citiesWithDepot[0] = depot;
    for (int i = 1; i < dim; ++i) {
        citiesWithDepot[i] = m_Cities[i - 1];
    }

    m_DistanceMatrix = std::vector(dim, std::vector(dim, 0));

    for (size_t i = 0; i < dim; ++i) {
        for (size_t j = i + 1; j < dim; ++j) {
            int dist = static_cast<int>(
                std::sqrt(
                    std::pow(citiesWithDepot[i].m_PosX - citiesWithDepot[j].m_PosX, 2)
                    + std::pow(citiesWithDepot[i].m_PosY - citiesWithDepot[j].m_PosY, 2))
                + 0.5);

            m_DistanceMatrix[i][j] = m_DistanceMatrix[j][i] = dist;
        }
    }

    // Calculate minimum distance vector
    m_MinDistanceVec = std::vector(dim, 0);
    for (size_t i = 0; i < dim; ++i) {
        int minDist = INT_MAX;
        for (size_t j = 0; j < dim; ++j) {
            if (i != j) {
                minDist = std::min(minDist, m_DistanceMatrix[i][j]);
            }
        }
        m_MinDistanceVec[i] = minDist;
    }
}
