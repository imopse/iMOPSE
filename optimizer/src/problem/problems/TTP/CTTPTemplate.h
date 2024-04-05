#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

#define USE_EOK 0

struct SCity
{
    SCity(const int &id, const float &x, const float &y);

    int m_ID;
    float m_PosX, m_PosY;
};

struct SItem
{
    SItem(const int &id, const int &profit, const int &weight, const int &nodeId);

    int m_ID, m_Profit, m_Weight, m_NodeId;
};

class CTTPTemplate
{
public:

    void Clear();

    const std::string &GetFileName() const
    { return m_FileName; }

    void SetFileName(const std::string &fileName)
    { m_FileName = fileName; }

    void SetData(const std::vector<SCity> &cities, const std::vector<SItem> &items, int capacity, float minSpeed,
                 float maxSpeed, float rentingRatio);

    const std::vector<SCity> &GetCities() const
    { return m_Cities; }

    const std::vector<SItem> &GetItems() const
    { return m_Items; }

    const std::vector<std::vector<float>> &GetDistMtx() const
    { return m_DistanceMatrix; }

    const std::vector<float> &GetMinDistVec() const
    { return m_MinDistanceVec; }

    const std::vector<std::vector<size_t>> &GetCityItems() const
    { return m_CityItems; }

    const std::vector<size_t> &GetProfitRatioSortedItems() const
    { return m_ProfitRatioSortedItems; }

    int GetCapacity() const
    { return m_Capacity; }

    float GetMinSpeed() const
    { return m_MinSpeed; }

    float GetMaxSpeed() const
    { return m_MaxSpeed; }

    float GetRentingRatio() const
    { return m_RentingRatio; }

    size_t GetCitiesSize() const
    { return m_Cities.size(); }

    size_t GetItemsSize() const
    { return m_Items.size(); }

    float GetMaxTravelTime() const;
    float GetMaxProfit() const;

    float GetMinTravelTime() const;
    float GetMinProfit() const;

private:

    float CalculateMaxDistance() const;
    float CalculateMinDistance() const;
    void CalculateContextData();

    std::string m_FileName;

    // File data
    std::vector<SCity> m_Cities;
    std::vector<SItem> m_Items;
    std::vector<size_t> m_ProfitRatioSortedItems;
    int m_Capacity;
    float m_MinSpeed;
    float m_MaxSpeed;
    float m_RentingRatio;

    // Context data
    std::vector<std::vector<float>> m_DistanceMatrix;
    std::vector<float> m_MinDistanceVec;
    std::vector<std::vector<size_t>> m_CityItems;
};
