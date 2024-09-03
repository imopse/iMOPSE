#include "CECVRPTWSolution.h"
#include "problem/problems/ECVRPTW/CECVRPTWTemplate.h"
#include <numeric>
#include <stdexcept>

CECVRPTWSolution::CECVRPTWSolution(CECVRPTWTemplate& problemTemplate)
    : m_ECVRPTWTemplate(problemTemplate)
{
}

float CECVRPTWSolution::GetTotalDistance() const
{
    return std::accumulate(m_Distance.begin(), m_Distance.end(), 0.f);
}

float CECVRPTWSolution::GetTotalDuration() const
{
    return std::accumulate(m_CurrentTime.begin(), m_CurrentTime.end(), 0.f);
}

void CECVRPTWSolution::BuildSolution(const std::vector<int>& initialAssignment)
{
    PrepareData(initialAssignment);

    auto& distMtx = m_ECVRPTWTemplate.GetDistInfoMtx();
    auto& cities = m_ECVRPTWTemplate.GetCities();

    int currentCar = 0;
    for (m_CurrentSolutionIdx = 0; m_CurrentSolutionIdx < m_Solution.size(); ++m_CurrentSolutionIdx)
    {
        size_t nextCityIdx = m_Solution[(m_CurrentSolutionIdx + 1) % m_Solution.size()];
        if (nextCityIdx == VEHICLE_DELIMITER)
        {
            currentCar++;
            continue;
        }

        if (CanSatisfyDemand(currentCar, nextCityIdx))
        {
            if (CanSafelyReach(currentCar, nextCityIdx))
            {
                MoveCarToNextCity(currentCar, nextCityIdx);
            }
            else
            {
                // We assume, we can safely reach the recharge station, as we always check beforehand
                MoveCarToRechargeStationTowardsCity(currentCar, nextCityIdx);
            }
        }
        else
        {
            size_t depotIdx = m_ECVRPTWTemplate.GetNearestDepotIdx(m_CurrentPosition[currentCar]);
            if (CanSafelyReach(currentCar, depotIdx))
            {
                // We assume, that we can safely reach any city (including recharge station) from the depot
                MoveCarToDepoLoadAndRecharge(currentCar, depotIdx);
            }
            else
            {
                // There is a chance, we will not reach the depot at the moment but must reach recharge station
                MoveCarToRechargeStationTowardsCity(currentCar, depotIdx);
            }
        }
    }
}

bool CECVRPTWSolution::CanSatisfyDemand(size_t carIdx, size_t cityIdx) const
{
    return m_CurrentLoad[carIdx] >= m_ECVRPTWTemplate.GetCities()[cityIdx].m_Demand;
}

bool CECVRPTWSolution::CanSafelyReach(size_t carIdx, size_t cityIdx) const
{
    float fuelToTarget = m_ECVRPTWTemplate.GetRequiredFuel(m_CurrentPosition[carIdx], cityIdx);
    size_t nearestChargingToCityIdx = m_ECVRPTWTemplate.GetNearestChargingStationIdx(cityIdx);
    float fuelFromTargetToNearestCharging = m_ECVRPTWTemplate.GetRequiredFuel(cityIdx, nearestChargingToCityIdx);
    return m_CurrentTankCapacity[carIdx] >= (fuelToTarget + fuelFromTargetToNearestCharging);
}

void CECVRPTWSolution::PrepareData(const std::vector<int>& initialAssignment)
{
    size_t vehicleCount = m_ECVRPTWTemplate.GetVehicleCount();
    m_CurrentLoad = std::vector<int>(vehicleCount, m_ECVRPTWTemplate.GetCapacity());
    m_CurrentPosition = std::vector<size_t>(vehicleCount, DEPOT_CITY_ID);
    m_Distance = std::vector<float>(vehicleCount, 0.f);
    m_CurrentTankCapacity = std::vector<float>(vehicleCount, m_ECVRPTWTemplate.GetTankCapcity());
    m_CurrentTime = std::vector<float>(vehicleCount, 0.f);

    m_Solution = initialAssignment;
    m_Solution.emplace(m_Solution.begin(), DEPOT_CITY_ID);
    m_Solution.emplace(m_Solution.end(), DEPOT_CITY_ID);
    for (size_t i = 0; i < m_Solution.size(); ++i)
    {
        if (m_Solution[i] == VEHICLE_DELIMITER)
        {
            m_Solution.emplace(m_Solution.begin() + i, DEPOT_CITY_ID);
            m_Solution.emplace(m_Solution.begin() + i + 2, DEPOT_CITY_ID);
            i += 2;
        }
    }
}

float CECVRPTWSolution::CalculateRefuelTime(float tankCapacity, float currentTankCapacity)
{
    return (tankCapacity - currentTankCapacity) / m_ECVRPTWTemplate.GetRefuelingRate();
}

void CECVRPTWSolution::MoveCarToDepoLoadAndRecharge(size_t carIdx, size_t depotIdx)
{
    auto& distMtx = m_ECVRPTWTemplate.GetDistInfoMtx();
    size_t& currentCityIdx = m_CurrentPosition[carIdx];

    //To depot
    m_Distance[carIdx] += distMtx[currentCityIdx][depotIdx].m_Distance;
    m_CurrentTime[carIdx] += distMtx[currentCityIdx][depotIdx].m_TravelTime;
    m_CurrentTankCapacity[carIdx] -= distMtx[currentCityIdx][depotIdx].m_FuelConsumption;

    //Depot refuel
    m_CurrentTime[carIdx] += CalculateRefuelTime(m_ECVRPTWTemplate.GetTankCapcity(), m_CurrentTankCapacity[carIdx]);
    m_CurrentTankCapacity[carIdx] = m_ECVRPTWTemplate.GetTankCapcity();

    //Depot car loading
    m_CurrentLoad[carIdx] = m_ECVRPTWTemplate.GetCapacity();

    //Add depot visit to solution
    m_Solution.emplace(m_Solution.begin() + (int)m_CurrentSolutionIdx + 1, depotIdx);
    currentCityIdx = depotIdx;
}

void CECVRPTWSolution::MoveCarToDepoLoadRechargeAndThenToCity(size_t carIdx, size_t depotIdx, size_t nextCityIdx)
{
    auto& distMtx = m_ECVRPTWTemplate.GetDistInfoMtx();
    auto& cities = m_ECVRPTWTemplate.GetCities();

    size_t& currentCityIdx = m_CurrentPosition[carIdx];

    //To depot
    m_Distance[carIdx] += distMtx[currentCityIdx][depotIdx].m_Distance;
    m_CurrentTime[carIdx] += distMtx[currentCityIdx][depotIdx].m_TravelTime;
    m_CurrentTankCapacity[carIdx] -= distMtx[currentCityIdx][depotIdx].m_FuelConsumption;

    //Depot refuel
    m_CurrentTime[carIdx] += CalculateRefuelTime(m_ECVRPTWTemplate.GetTankCapcity(), m_CurrentTankCapacity[carIdx]);
    m_CurrentTankCapacity[carIdx] = m_ECVRPTWTemplate.GetTankCapcity();

    //Depot car loading
    m_CurrentLoad[carIdx] = m_ECVRPTWTemplate.GetCapacity();

    //To next city
    m_Distance[carIdx] += distMtx[depotIdx][nextCityIdx].m_Distance;
    m_CurrentTime[carIdx] += distMtx[depotIdx][nextCityIdx].m_TravelTime;
    m_CurrentTankCapacity[carIdx] -= distMtx[depotIdx][nextCityIdx].m_FuelConsumption;

    m_CurrentLoad[carIdx] -= cities[nextCityIdx].m_Demand;

    //Add depot visit to solution
    m_Solution.emplace(m_Solution.begin() + (int)m_CurrentSolutionIdx + 1, depotIdx);

    HandleTimeOnCity(carIdx, nextCityIdx);
    m_CurrentSolutionIdx++;
    currentCityIdx = nextCityIdx;
}

void CECVRPTWSolution::MoveCarToNextCity(size_t carIdx, size_t nextCityIdx)
{
    auto& distMtx = m_ECVRPTWTemplate.GetDistInfoMtx();
    auto& cities = m_ECVRPTWTemplate.GetCities();

    size_t& currentCityIdx = m_CurrentPosition[carIdx];
    m_Distance[carIdx] += distMtx[currentCityIdx][nextCityIdx].m_Distance;
    m_CurrentTime[carIdx] += distMtx[currentCityIdx][nextCityIdx].m_TravelTime;
    m_CurrentTankCapacity[carIdx] -= distMtx[currentCityIdx][nextCityIdx].m_FuelConsumption;
    m_CurrentLoad[carIdx] -= cities[nextCityIdx].m_Demand;

    HandleTimeOnCity(carIdx, nextCityIdx);

    m_CurrentTime[carIdx] += cities[nextCityIdx].m_ServiceTime;
    currentCityIdx = nextCityIdx;
}

void CECVRPTWSolution::HandleTimeOnCity(size_t carIdx, size_t nextCityIdx)
{
    auto& cities = m_ECVRPTWTemplate.GetCities();
    auto dayLength = m_ECVRPTWTemplate.GetMaxDueTime();

    if (std::fmod(m_CurrentTime[carIdx], dayLength) < cities[nextCityIdx].m_ReadyTime)
    {
        m_CurrentTime[carIdx] += cities[nextCityIdx].m_ReadyTime - std::fmod(m_CurrentTime[carIdx], dayLength);
    }
    else if (std::fmod(m_CurrentTime[carIdx], dayLength) > cities[nextCityIdx].m_DueTime)
    {
        float timeToEndOfDay = dayLength - std::fmod(m_CurrentTime[carIdx], dayLength);
        m_CurrentTime[carIdx] += timeToEndOfDay;
        m_CurrentTime[carIdx] += cities[nextCityIdx].m_ReadyTime;
    }
}

void CECVRPTWSolution::MoveCarToNearestRechargeStation(size_t carIdx)
{
    size_t nearestChargingStationIdx = m_ECVRPTWTemplate.GetNearestChargingStationIdx(m_CurrentPosition[carIdx]);
    MoveCarToRechargeStation(carIdx, nearestChargingStationIdx);
}

void CECVRPTWSolution::MoveCarToRechargeStationTowardsCity(size_t carIdx, size_t nextCityIdx)
{
    auto& chargingStations = m_ECVRPTWTemplate.GetChargingStations();
    auto& distMtx = m_ECVRPTWTemplate.GetDistInfoMtx();
    size_t currentCityIdx = m_CurrentPosition[carIdx];

    // this is simple approach but works as expected - get nearest to target, reachable station
    float bestDist = FLT_MAX;
    size_t bestStationIdx = distMtx.size();
    float currentFuel = m_CurrentTankCapacity[carIdx];
    for (size_t stationIdx : chargingStations)
    {
        float d = distMtx[stationIdx][nextCityIdx].m_Distance;
        float f = distMtx[currentCityIdx][stationIdx].m_FuelConsumption;
        if (d < bestDist && f <= currentFuel)
        {
            bestDist = d;
            bestStationIdx = stationIdx;
        }
    }

    if (bestStationIdx >= distMtx.size())
    {
        throw std::runtime_error("Cannot find reachable station!");
    }

    MoveCarToRechargeStation(carIdx, bestStationIdx);
}

void CECVRPTWSolution::MoveCarToRechargeStation(size_t carIdx, size_t stationIdx)
{
    auto& distMtx = m_ECVRPTWTemplate.GetDistInfoMtx();

    size_t& currentCityIdx = m_CurrentPosition[carIdx];
    m_Distance[carIdx] += distMtx[currentCityIdx][stationIdx].m_Distance;
    m_CurrentTime[carIdx] += distMtx[currentCityIdx][stationIdx].m_TravelTime;
    m_CurrentTime[carIdx] += CalculateRefuelTime(m_ECVRPTWTemplate.GetTankCapcity(), m_CurrentTankCapacity[carIdx]);
    m_CurrentTankCapacity[carIdx] = m_ECVRPTWTemplate.GetTankCapcity();
    m_Solution.emplace(m_Solution.begin() + (int)m_CurrentSolutionIdx + 1, stationIdx);
    currentCityIdx = stationIdx;
    if (currentCityIdx == DEPOT_CITY_ID) // TODO - handle to not use index
    {
        m_CurrentLoad[carIdx] = m_ECVRPTWTemplate.GetCapacity();
    }
}