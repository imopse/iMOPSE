#include <iostream>
#include "CECVRPTWTemplate.h"
#include <cstddef>

SCityECVRPTW::SCityECVRPTW(const int& id
	, const std::string& strId
	, const ENodeType type
	, const float& x
	, const float& y
	, const int& demand
	, const float& readyTime
	, const float& dueTime
	, const float& serviceTime
) : m_StrID(strId)
{
	m_ID = id;
	m_Type = type;
	m_PosX = x;
	m_PosY = y;
	m_Demand = demand;
	m_ReadyTime = readyTime;
	m_DueTime = dueTime;
	m_ServiceTime = serviceTime;
}

void CECVRPTWTemplate::Clear()
{
	m_Capacity = 0;
	m_AverageVelocity = 0;
	m_FuelConsumptionRate = 0;
	m_RefuelingRate = 0;
	m_TankCapacity = 0;

    m_Cities.clear();
    m_ChargingStationIndexes.clear();
    m_DepotIndexes.clear();
    m_CustomerIndexes.clear();
	m_DistanceInfoMatrix.clear();
	m_MinDistanceVec.clear();
}

void CECVRPTWTemplate::SetData(std::vector<SCityECVRPTW>& cities
	, int capacity
	, float tankCapacity
	, float fuelConsumptionRate
	, float refuelingRate
	, float averageVelocity
	, std::vector<std::size_t>& chargingStationIndexes
	, std::vector<std::size_t>& depotIndexes
	, std::vector<std::size_t>& customerIndexes
	, int vehicleCount
)
{
	Clear();

	m_Cities = cities;
	m_Capacity = capacity;
	m_DepotIndexes = depotIndexes;
	m_ChargingStationIndexes = chargingStationIndexes;
    m_CustomerIndexes = customerIndexes;
	m_TankCapacity = tankCapacity;
	m_FuelConsumptionRate = fuelConsumptionRate;
	m_RefuelingRate = refuelingRate;
	m_AverageVelocity = averageVelocity;
	m_VehicleCount = vehicleCount;

	CalculateContextData();
}

float CECVRPTWTemplate::GetMinDistance() const {
	float dist = 0.f;
	std::size_t dim = m_DistanceInfoMatrix.size();
	for (std::size_t i = 0; i < dim; ++i)
	{
		float minDist = FLT_MAX;
		for (std::size_t j = 0; j < dim; ++j)
		{
			if (i != j)
			{
				minDist = fminf(minDist, m_DistanceInfoMatrix[i][j].m_Distance);
			}
		}
		dist += minDist;
	}
	return dist;
}

float CECVRPTWTemplate::GetMaxDistance() const {
	float dist = 0.f;
	std::size_t dim = m_DistanceInfoMatrix.size();
	for (std::size_t i = 0; i < dim; ++i)
	{
		float maxDist = FLT_MIN;
		for (std::size_t j = 0; j < dim; ++j)
		{
			if (i != j)
			{
				maxDist = fmaxf(maxDist, m_DistanceInfoMatrix[i][j].m_Distance);
			}
		}
		dist += maxDist;
	}
	return dist;
}

float CECVRPTWTemplate::GetMaxTimeService() const {
	float maxTime = FLT_MIN;
	std::size_t dim = m_Cities.size();
	for (std::size_t i = 0; i < dim; ++i)
	{
		if (maxTime < m_Cities[i].m_ServiceTime) {
			maxTime = m_Cities[i].m_ServiceTime;
		}
	}
	return maxTime;
}

float CECVRPTWTemplate::GetRequiredFuel(std::size_t cityIdx, std::size_t nextCityIdx) const
{
    return m_DistanceInfoMatrix[cityIdx][nextCityIdx].m_FuelConsumption;
}

std::size_t CECVRPTWTemplate::GetNearestDepotIdx(std::size_t cityIdx) const
{
    // TODO - cache

    float minDist = FLT_MAX;
    std::size_t chosenIdx;
    auto& distMtx = GetDistInfoMtx();
    auto& depotIndexes = GetDepots();
    auto& cities = GetCities();

    for (const auto idx : depotIndexes)
    {
        int depotIndex;
        for (int i = 0; i < cities.size(); i++)
        {
            if (cities[i].m_ID == idx)
            {
                depotIndex = i;
                break;
            }
        }
        if (distMtx[cityIdx][depotIndex].m_Distance < minDist)
        {
            chosenIdx = depotIndex;
            minDist = distMtx[cityIdx][depotIndex].m_Distance;
        }
    }
    return chosenIdx;
}

std::size_t CECVRPTWTemplate::GetNearestChargingStationIdx(std::size_t cityIdx) const
{
    // TODO - cache
    
    float minDist = FLT_MAX;
    std::size_t chosenIdx;
    auto& distMtx = GetDistInfoMtx();
    auto& depotIndexes = GetChargingStations();
    auto& cities = GetCities();

    for (const auto idx : depotIndexes)
    {
        int depotIndex;
        for (int i = 0; i < cities.size(); i++)
        {
            if (cities[i].m_ID == idx)
            {
                depotIndex = i;
                break;
            }
        }
        if (distMtx[cityIdx][depotIndex].m_Distance < minDist)
        {
            chosenIdx = depotIndex;
            minDist = distMtx[cityIdx][depotIndex].m_Distance;
        }
    }
    return chosenIdx;
}

bool CECVRPTWTemplate::Validate() const
{
    bool isValid = true;
    for (std::size_t i = 0; i < m_DistanceInfoMatrix.size(); ++i)
    {
        if (m_Cities[i].m_Type == ENodeType::ChargingStation)
        {
            std::size_t depotIdx = GetNearestDepotIdx(i);
            // depot can be reached from any charging station
            if (m_DistanceInfoMatrix[i][depotIdx].m_FuelConsumption > m_TankCapacity)
            {
                std::cout << "Depot cannot be reached from the charging station [" << i << "]!" << std::endl;
                isValid = false;
            }
        }
        std::size_t chargingStationIdx = GetNearestChargingStationIdx(i);
        // any city can be reached from the charging station and back
        if ((m_DistanceInfoMatrix[chargingStationIdx][i].m_FuelConsumption + m_DistanceInfoMatrix[i][chargingStationIdx].m_FuelConsumption) > m_TankCapacity)
        {
            std::cout << "City [" << i << "] cannot be safely reached from the nearest charging station!" << std::endl;
            isValid = false;
        }
    }
    return isValid;
}

void CECVRPTWTemplate::CalculateContextData()
{
	std::size_t dim = m_Cities.size();
	m_DistanceInfoMatrix = std::vector<std::vector<SDistanceInfo>>(dim, std::vector<SDistanceInfo>(dim, SDistanceInfo{0, 0, 0}));
	for (std::size_t i = 0; i < dim; ++i)
	{
		for (std::size_t j = 0; j < dim; ++j)
		{
			float dist = sqrtf(powf(m_Cities[i].m_PosX - m_Cities[j].m_PosX, 2) + powf(m_Cities[i].m_PosY - m_Cities[j].m_PosY, 2));
			float time = dist / m_AverageVelocity;
			float fuelConsumptionRate = dist * m_FuelConsumptionRate;
			m_DistanceInfoMatrix[i][j] = SDistanceInfo
			{
				dist,
				time,
				fuelConsumptionRate
			};
		}
	}

	// Calculate minimum distance vector
	m_MinDistanceVec = std::vector<float>(dim, 0.f);
	for (std::size_t i = 0; i < dim; ++i)
	{
		float minDist = FLT_MAX;
		for (std::size_t j = 0; j < dim; ++j)
		{
			if (i != j)
			{
				minDist = fminf(minDist, m_DistanceInfoMatrix[i][j].m_Distance);
			}
		}
		m_MinDistanceVec[i] = minDist;
	}
}
