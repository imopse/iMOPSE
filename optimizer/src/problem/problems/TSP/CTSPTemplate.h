#include <vector>
#include <cmath>
#include "CCity.h"

class CTSPTemplate {
public:
    std::vector<CCity> m_Cities;
    std::vector<std::vector<float>> m_DistanceMatrix;
    float m_MaxDistance = 0;

    void CalculateDistanceMatrix() {
        size_t numCities = m_Cities.size();
        m_DistanceMatrix.resize(numCities, std::vector<float>(numCities, 0.f));

        for (size_t i = 0; i < numCities; i++) {
            for (size_t j = 0; j < numCities; j++) {
                if (i != j) {
                    m_DistanceMatrix[i][j] = CalculateDistance(m_Cities[i], m_Cities[j]);
                }
            }
        }
    }

    float CalculateDistance(const CCity &city1, const CCity &city2) {
        return std::sqrt(std::pow(city1.m_PosX - city2.m_PosX, 2) + std::pow(city1.m_PosY - city2.m_PosY, 2));
    }
    
    void SetCities(std::vector<CCity> &cities) {
        m_Cities = cities;
        CalculateDistanceMatrix();
    }

    const std::vector<CCity>& GetCities() const {
        return m_Cities;
    }

    size_t GetCitiesSize() const {
        return m_Cities.size();
    }

    const std::vector<std::vector<float>>& GetDistMtx() const {
        return m_DistanceMatrix;
    }

    void CalculateMaxDistance() {
        for (const auto& row : m_DistanceMatrix) {
            for (float dist : row) {
                m_MaxDistance = std::max(m_MaxDistance, dist);
            }
        }
        
        m_MaxDistance *= m_Cities.size();
    }
};