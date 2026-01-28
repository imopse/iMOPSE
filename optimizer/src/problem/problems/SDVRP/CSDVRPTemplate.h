#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cfloat>

struct SCitySDVRP {
    SCitySDVRP() = default;

    SCitySDVRP(const int &id, const int &index, const float &x, const float &y, const int &demand);

    int m_ID;
    int m_index;
    float m_PosX, m_PosY;
    int m_demand;
};

class CSDVRPTemplate {
public:
    void Clear();

    const std::string &GetFileName() const { return m_FileName; }
    void SetFileName(const std::string &fileName) { m_FileName = fileName; }

    void SetData(const std::vector<SCitySDVRP> &cities, int capacity, int trucks, const SCitySDVRP &depot);

    const std::vector<SCitySDVRP> &GetCities() const { return m_Cities; }
    const std::vector<std::vector<int> > &GetDistMtx() const { return m_DistanceMatrix; }
    const std::vector<int> &GetMinDistVec() const { return m_MinDistanceVec; }
    const SCitySDVRP &GetDepot() const { return m_Depot; }

    int GetCapacity() const { return m_Capacity; }
    int GetTrucks() const { return m_Trucks; }

    size_t GetCitiesSize() const { return m_Cities.size(); }

    int GetMinDistance() const;

    int GetMaxDistance() const;

private:
    void CalculateContextData();

    std::string m_FileName;

    // File data
    std::vector<SCitySDVRP> m_Cities;
    SCitySDVRP m_Depot = {};
    int m_Capacity = 0;
    int m_Trucks = 0;

    // Context data
    std::vector<std::vector<int> > m_DistanceMatrix;
    std::vector<int> m_MinDistanceVec;
};
