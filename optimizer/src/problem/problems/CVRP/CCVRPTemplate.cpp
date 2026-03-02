#include "CCVRPTemplate.h"

SCityCVRP::SCityCVRP(const int& id, const float& x, const float& y, const int& demand)
{
	m_ID = id;
	m_PosX = x;
	m_PosY = y;
	m_demand=demand;
}

void CCVRPTemplate::Clear()
{
	m_Cities.clear();
	m_Capacity = 0;
	m_Trucks = 0;

	m_DistanceMatrix.clear();
	m_MinDistanceVec.clear();
	m_NearestDepotCache.clear();
	m_NearestNeighborsCache.clear();
}

void CCVRPTemplate::SetData(const std::vector<SCityCVRP>& cities, int capacity,int trucks,const std::vector<size_t>& depotIndexes)
{
	Clear();

	m_Cities = cities;
	m_Capacity = capacity;
	m_Trucks = trucks;
	m_DepotIndexes=depotIndexes;

	CalculateContextData();
	CalculateNearestDepotCache();
	CalculateNearestNeighborsCache();
}

float CCVRPTemplate::GetMinDistance() const {
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

float CCVRPTemplate::GetMaxDistance() const {
	return GetMinDistance() * 2.f;
}


void CCVRPTemplate::CalculateContextData()
{
	size_t dim = m_Cities.size();
	m_DistanceMatrix = std::vector<std::vector<float>>(dim, std::vector<float>(dim, 0.f));
	for (size_t i = 0; i < dim; ++i)
	{
		for (size_t j = i + 1; j < dim; ++j)
		{
			// Use ceil distance
			float dist = ceilf(sqrtf(powf(m_Cities[i].m_PosX - m_Cities[j].m_PosX, 2) + powf(m_Cities[i].m_PosY - m_Cities[j].m_PosY, 2)));
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
}

void CCVRPTemplate::CalculateNearestDepotCache()
{
	size_t dim = m_Cities.size();
	m_NearestDepotCache.resize(dim);
	
	// Pre-compute nearest depot for each city
	for (size_t cityIdx = 0; cityIdx < dim; ++cityIdx)
	{
		float minDist = FLT_MAX;
		size_t chosenIdx = 0;
		
		for (const auto depotId : m_DepotIndexes)
		{
			for (size_t i = 0; i < m_Cities.size(); ++i)
			{
				if (m_Cities[i].m_ID == depotId)
				{
					if (m_DistanceMatrix[cityIdx][i] < minDist)
					{
						chosenIdx = i;
						minDist = m_DistanceMatrix[cityIdx][i];
					}
					break;
				}
			}
		}
		m_NearestDepotCache[cityIdx] = chosenIdx;
	}
}

void CCVRPTemplate::CalculateNearestNeighborsCache()
{
	size_t dim = m_Cities.size();
	m_NearestNeighborsCache.resize(dim);
	
	// For each city, store sorted list of all other cities by distance
	for (size_t i = 0; i < dim; ++i)
	{
		std::vector<std::pair<float, size_t>> distances;
		distances.reserve(dim - 1);
		
		for (size_t j = 0; j < dim; ++j)
		{
			if (i != j)
			{
				distances.emplace_back(m_DistanceMatrix[i][j], j);
			}
		}
		
		// Sort by distance (ascending)
		std::sort(distances.begin(), distances.end());
		
		m_NearestNeighborsCache[i].reserve(distances.size());
		for (const auto& [dist, idx] : distances)
		{
			m_NearestNeighborsCache[i].push_back(idx);
		}
	}
}
