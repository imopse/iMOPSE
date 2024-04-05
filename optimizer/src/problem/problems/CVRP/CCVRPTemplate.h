#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cfloat>

struct SCityCVRP {
    SCityCVRP(const int& id, const float& x, const float& y, const int& demand);

    int m_ID;
    float m_PosX, m_PosY;
    int m_demand;
};

class CCVRPTemplate {
public:

    void Clear();
    const std::string& GetFileName() const { return m_FileName; }
    void SetFileName(const std::string& fileName) { m_FileName = fileName; }
    void SetData(const std::vector<SCityCVRP>& cities, int capacity,int trucks,const std::vector<size_t>& depotIndexes);

    const std::vector<SCityCVRP>& GetCities() const { return m_Cities; }
    const std::vector<std::vector<float>>& GetDistMtx() const { return m_DistanceMatrix; }
    const std::vector<float>& GetMinDistVec() const { return m_MinDistanceVec; }
    const std::vector<size_t>& GetDepots()const { return m_DepotIndexes; }

    int GetCapacity() const { return m_Capacity; }
    int GetTrucks() const { return m_Trucks; }

    size_t GetCitiesSize() const { return m_Cities.size(); }

    float GetMinDistance() const;
    float GetMaxDistance() const;

private:
    void CalculateContextData();

    std::string m_FileName;

    // File data
    std::vector<SCityCVRP> m_Cities;
    std::vector<size_t> m_DepotIndexes;
    int m_Capacity;
    int m_Trucks;

    // Context data
    std::vector<std::vector<float>> m_DistanceMatrix;
    std::vector<float> m_MinDistanceVec;
};
