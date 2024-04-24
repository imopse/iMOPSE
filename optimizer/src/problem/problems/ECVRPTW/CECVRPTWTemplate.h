#pragma once

#define PENALTYMULTIPLIER 5
#define DISTANCE_WEIGHT 20
#define TIME_WEIGHT 10
#define COST_WEIGHT 1

#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cfloat>
#include "ENodeType.h"

constexpr int VEHICLE_DELIMITER = INT32_MAX;

struct SCityECVRPTW {
    SCityECVRPTW(const int& id
        , const std::string& strId
        , const ENodeType type
        , const float& x
        , const float& y
        , const int& demand
        , const float& readyTime
        , const float& dueTime
        , const float& serviceTime
    );

    int m_ID;
    ENodeType m_type;
    const std::string& m_StrID;
    float m_PosX, m_PosY;
    int m_demand;
    float m_readyTime, m_dueTime, m_serviceTime;
};

struct SDistanceInfo
{
    float m_distance;
    float m_travelTime;
    float m_fuelConsumption;
};

class CECVRPTWTemplate {
public:

    void Clear();
    const std::string& GetFileName() const { return m_FileName; }
    void SetFileName(const std::string& fileName) { m_FileName = fileName; }
    void SetData(std::vector<SCityECVRPTW>& cities
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
    );

    const std::vector<SCityECVRPTW>& GetCities() const { return *m_Cities; }
    const std::vector<std::vector<SDistanceInfo>>& GetDistInfoMtx() const { return m_DistanceInfoMatrix; }
    const std::vector<float>& GetMinDistVec() const { return m_MinDistanceVec; }
    const std::vector<size_t>& GetDepots() const { return *m_DepotIndexes; }
    const std::vector<size_t>& GetChargingStations() const { return *m_ChargingStationIndexes; }
    const std::vector<size_t>& GetCustomers() const { return *m_CutomerIndexes; }

    int GetCapacity() const { return m_Capacity; }
    int GetTrucks() const { return m_Trucks; }
    float GetTankCapcity() const { return m_TankCapacity; }
    float GetFuelConsumptionRate() const { return m_FuelConsumptionRate; }
    float GetRefuelingRate() const { return m_RefuelingRate; }
    float GetAverageVelociy() const { return m_averageVelocity; }
    int GetVehicleCount() const { return (int)m_vehicleCount; }

    size_t GetCitiesSize() const { return m_Cities->size(); }

    float GetMinDistance() const;
    float GetMaxDistance() const;
    float GetMaxTimeService() const;
    float GetMaxDueTime() const { return (*m_Cities)[0].m_dueTime; }
    float GetMaxCost() const { 
        return (this->GetMaxDistance() * DISTANCE_WEIGHT
            + this->GetMaxDueTime()  * TIME_WEIGHT
            + fminf(this->GetMaxTimeService() * PENALTYMULTIPLIER, powf(this->GetMaxDueTime(), 2)) * m_Cities->size() * COST_WEIGHT) * this->GetVehicleCount();
    }

private:
    void CalculateContextData();

    std::string m_FileName;

    // File data
    std::vector<SCityECVRPTW>* m_Cities;
    std::vector<size_t>* m_DepotIndexes;
    std::vector<size_t>* m_ChargingStationIndexes;
    std::vector<size_t>* m_CutomerIndexes;
    int m_Capacity;
    int m_Trucks;
    float m_TankCapacity;
    float m_FuelConsumptionRate;
    float m_RefuelingRate;
    float m_averageVelocity;
    float m_vehicleCount;

    // Context data
    std::vector<std::vector<SDistanceInfo>> m_DistanceInfoMatrix;
    std::vector<float> m_MinDistanceVec;
};
