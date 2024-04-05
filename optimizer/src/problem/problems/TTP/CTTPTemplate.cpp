#include "CTTPTemplate.h"
#include <algorithm>
#include <cfloat>
#include "TTPJavaEvalData.h"

#define USE_JAVA_MAXPROFIT 1

SCity::SCity(const int &id, const float &x, const float &y)
{
    m_ID = id;
    m_PosX = x;
    m_PosY = y;
}

SItem::SItem(const int &id, const int &profit, const int &weight, const int &nodeId)
{
    m_ID = id;
    m_Profit = profit;
    m_Weight = weight;
    m_NodeId = nodeId;
}

void CTTPTemplate::Clear()
{
    m_Cities.clear();
    m_Items.clear();
    m_Capacity = 0;
    m_MinSpeed = 0;
    m_MaxSpeed = 0;
    m_RentingRatio = 0;

    m_DistanceMatrix.clear();
    m_MinDistanceVec.clear();
    m_CityItems.clear();
    m_ProfitRatioSortedItems.clear();
}

void
CTTPTemplate::SetData(const std::vector<SCity> &cities, const std::vector<SItem> &items, int capacity, float minSpeed,
                      float maxSpeed, float rentingRatio)
{
    Clear();

    m_Cities = cities;
    m_Items = items;
    m_Capacity = capacity;
    m_MinSpeed = minSpeed;
    m_MaxSpeed = maxSpeed;
    m_RentingRatio = rentingRatio;

    CalculateContextData();
}

float CTTPTemplate::GetMaxTravelTime() const
{
    return CalculateMaxDistance() / m_MinSpeed;
}

float CTTPTemplate::GetMaxProfit() const
{
//#if USE_JAVA_MAXPROFIT
//    return g_TTPMaxProfit[m_FileName];
//#else
    std::vector<SItem> itemsCopy = m_Items;

    auto sortLambda = [](const SItem& lhv, const SItem& rhv) -> bool
    {
        // float r = (float)rhv.profit / (float)rhv.weight;
        // float l = (float)lhv.profit / (float)lhv.weight;
        // return r != l ? r < l : rhv.profit < lhv.profit;
        return ((float)rhv.m_Profit / (float)rhv.m_Weight) < ((float)lhv.m_Profit / (float)lhv.m_Weight);
    };

    std::sort(itemsCopy.begin(), itemsCopy.end(), sortLambda);

    size_t i = 0;
    int cap = itemsCopy[0].m_Weight;
    int maxProfit = itemsCopy[0].m_Profit;
    while (cap < m_Capacity && i < itemsCopy.size())
    {
        maxProfit += itemsCopy[i].m_Profit;
        cap += itemsCopy[i].m_Weight;
        ++i;
    }
    return (float)maxProfit;
//#endif
}

float CTTPTemplate::GetMinTravelTime() const
{
    return CalculateMinDistance() / m_MaxSpeed;
}

float CTTPTemplate::GetMinProfit() const
{
    return 0.f;
}

float CTTPTemplate::CalculateMaxDistance() const
{
    // Based on Java impl
    return CalculateMinDistance() * 2.f;
}

float CTTPTemplate::CalculateMinDistance() const
{
    float dist = 0.f;
    size_t dim = m_DistanceMatrix.size();
    for (size_t i = 0; i < dim; ++i)
    {
        float minDist = FLT_MAX;
        for (size_t j = 0; j < dim; ++j)
        {
            if (i != j)
            {
                minDist = fminf(minDist, m_DistanceMatrix[i][j]);
            }
        }
        dist += minDist;
    }
    return dist;
}

void CTTPTemplate::CalculateContextData()
{
    size_t dim = m_Cities.size();
    m_DistanceMatrix = std::vector<std::vector<float>>(dim, std::vector<float>(dim, 0.f));
    for (size_t i = 0; i < dim; ++i)
    {
        for (size_t j = i + 1; j < dim; ++j)
        {
            // Use ceil distance
            float dist = ceilf(sqrtf(powf(m_Cities[i].m_PosX - m_Cities[j].m_PosX, 2) +
                                     powf(m_Cities[i].m_PosY - m_Cities[j].m_PosY, 2)));
            m_DistanceMatrix[i][j] = m_DistanceMatrix[j][i] = dist;
        }
    }

    // Calculate minimum distance vector
    m_MinDistanceVec = std::vector<float>(dim, 0.f);
    for (size_t i = 0; i < dim; ++i)
    {
        float minDist = FLT_MAX;
        for (size_t j = 0; j < dim; ++j)
        {
            if (i != j)
            {
                minDist = fminf(minDist, m_DistanceMatrix[i][j]);
            }
        }
        m_MinDistanceVec[i] = minDist;
    }

    m_CityItems = std::vector<std::vector<size_t>>(dim, std::vector<size_t>());
    for (const SItem &item: m_Items)
    {
#if USE_EOK
        if (item.id == 0) continue;
#endif
        // Convert id to index (we assume they are ordered)
        m_CityItems[item.m_NodeId - 1].emplace_back(item.m_ID - 1);
    }

    // Sort by best profit / weight to worst
    std::vector<std::pair<float, size_t>> itemsRatio;
    itemsRatio.reserve(m_Items.size());
    for (size_t i = 0; i < m_Items.size(); ++i)
    {
#if USE_EOK
        if (m_Items[i].id == 0)
        {
            itemsRatio.emplace_back(0.f, i);
            continue;
        }
#endif
        const SItem &item = m_Items[i];
        itemsRatio.emplace_back((float) item.m_Profit / (float) item.m_Weight, i);
    }

    auto sortLambda = [](const std::pair<float, size_t> &lhv, const std::pair<float, size_t> &rhv) -> bool
    {
        return rhv.first < lhv.first;
    };

    std::sort(itemsRatio.begin(), itemsRatio.end(), sortLambda);

    m_ProfitRatioSortedItems.clear();
    m_ProfitRatioSortedItems.reserve(itemsRatio.size());
    for (const std::pair<float, size_t> &itemRatio: itemsRatio)
    {
        m_ProfitRatioSortedItems.push_back(itemRatio.second);
    }
}
