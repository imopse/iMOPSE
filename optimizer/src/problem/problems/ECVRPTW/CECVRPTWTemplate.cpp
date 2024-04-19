#include "CECVRPTWTemplate.h"

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
	m_type = type;
	m_PosX = x;
	m_PosY = y;
	m_demand = demand;
	m_readyTime = readyTime;
	m_dueTime = dueTime;
	m_serviceTime = serviceTime;
}

void CECVRPTWTemplate::Clear()
{
	m_Capacity = 0;
	m_Trucks = 0;
	m_averageVelocity = 0;
	m_FuelConsumptionRate = 0;
	m_RefuelingRate = 0;
	m_TankCapacity = 0;

	if (m_Cities != nullptr) {
		m_Cities->clear();
		delete m_Cities;
	}
	if (m_ChargingStationIndexes != nullptr) {
		m_ChargingStationIndexes->clear();
		delete m_ChargingStationIndexes;
	}
	if (m_DepotIndexes != nullptr) {
		m_DepotIndexes->clear();
		delete m_DepotIndexes;
	}
	if (m_CutomerIndexes != nullptr) {
		m_CutomerIndexes->clear();
		delete m_CutomerIndexes;
	}
	m_DistanceInfoMatrix.clear();
	m_MinDistanceVec.clear();
}

void CECVRPTWTemplate::SetData(std::vector<SCityECVRPTW>& cities
	, int capacity
	, int trucks
	, float tankCapacity
	, float fuelConsumptionRate
	, float refuelingRate
	, float averageVelocity
	, std::vector<size_t>& chargingStationIndexes
	, std::vector<size_t>& depotIndexes
	, std::vector<size_t>& customerIndexes
	, float vehicleCount
)
{
	Clear();

	m_Cities = &cities;
	m_Capacity = capacity;
	m_Trucks = trucks;
	m_DepotIndexes = &depotIndexes;
	m_ChargingStationIndexes = &chargingStationIndexes;
	m_CutomerIndexes = &customerIndexes;
	m_TankCapacity = tankCapacity;
	m_FuelConsumptionRate = fuelConsumptionRate;
	m_RefuelingRate = refuelingRate;
	m_averageVelocity = averageVelocity;
	m_vehicleCount = vehicleCount;

	CalculateContextData();
}

float CECVRPTWTemplate::GetMinDistance() const {
	float dist = 0.f;
	size_t dim = m_DistanceInfoMatrix.size();
	for (size_t i = 0; i < dim; ++i)
	{
		float minDist = FLT_MAX;
		for (size_t j = 0; j < dim; ++j)
		{
			if (i != j)
			{
				minDist = fminf(minDist, m_DistanceInfoMatrix[i][j].m_distance);
			}
		}
		dist += minDist;
	}
	return dist;
}

float CECVRPTWTemplate::GetMaxDistance() const {
	float dist = 0.f;
	size_t dim = m_DistanceInfoMatrix.size();
	for (size_t i = 0; i < dim; ++i)
	{
		float maxDist = FLT_MIN;
		for (size_t j = 0; j < dim; ++j)
		{
			if (i != j)
			{
				maxDist = fmaxf(maxDist, m_DistanceInfoMatrix[i][j].m_distance);
			}
		}
		dist += maxDist;
	}
	return dist;
}

float CECVRPTWTemplate::GetMaxTimeService() const {
	float maxTime = FLT_MIN;
	size_t dim = m_Cities->size();
	for (size_t i = 0; i < dim; ++i)
	{
		if (maxTime < (*m_Cities)[i].m_serviceTime) {
			maxTime = (*m_Cities)[i].m_serviceTime;
		}
	}
	return maxTime;
}

void CECVRPTWTemplate::CalculateContextData()
{
	size_t dim = m_Cities->size();
	m_DistanceInfoMatrix = std::vector<std::vector<SDistanceInfo>>(dim, std::vector<SDistanceInfo>(dim, SDistanceInfo{0, 0, 0}));
	for (size_t i = 0; i < dim; ++i)
	{
		for (size_t j = i + 1; j < dim; ++j)
		{
			float dist = sqrtf(powf((*m_Cities)[i].m_PosX - (*m_Cities)[j].m_PosX, 2) + powf((*m_Cities)[i].m_PosY - (*m_Cities)[j].m_PosY, 2));
			float time = dist / m_averageVelocity;
			float fuelConsumptionRate = dist * m_FuelConsumptionRate;
			m_DistanceInfoMatrix[i][j] = m_DistanceInfoMatrix[j][i] = SDistanceInfo
			{
				dist, //m_distance
				time, //m_travelTime
				fuelConsumptionRate //m_fuelConsumption
			};
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
				minDist = fminf(minDist, m_DistanceInfoMatrix[i][j].m_distance);
			}
		}
		m_MinDistanceVec[i] = minDist;
	}


}
