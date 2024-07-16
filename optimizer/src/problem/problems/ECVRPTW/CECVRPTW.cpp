#include "CECVRPTW.h"
#include "../../../utils/logger/CExperimentLogger.h"
#include <iostream>
#include <sstream>

CECVRPTW::CECVRPTW(CECVRPTWTemplate& ecvrptwBase) : m_ECVRPTWTemplate(ecvrptwBase)
{
    CreateProblemEncoding();

    m_MaxObjectiveValues = {
        m_ECVRPTWTemplate.GetMaxDistance(),
        m_ECVRPTWTemplate.GetMaxDueTime() * m_ECVRPTWTemplate.GetCustomers().size(),
        200
    };

    m_MinObjectiveValues = {
            0, //min distance
            0, //min due time
            0, //min cost
    };

    m_genotypeCpy = new std::vector<int>();
    m_genotypeCpy->reserve(GENOTYPEBUFFERSIZE);

    m_currentLoad = new std::vector<int>(m_ECVRPTWTemplate.GetVehicleCount());
    m_distance = new std::vector<float>(m_ECVRPTWTemplate.GetVehicleCount());
    m_currentTankCapacity = new std::vector<float>(m_ECVRPTWTemplate.GetVehicleCount());
    m_currentTime = new std::vector<float>(m_ECVRPTWTemplate.GetVehicleCount());
    m_additionalCost = new std::vector<float>(m_ECVRPTWTemplate.GetVehicleCount());
}

CECVRPTW::~CECVRPTW()
{
    m_genotypeCpy->clear();
    delete m_genotypeCpy;

    m_currentLoad->clear();
    m_distance->clear();
    m_currentTankCapacity->clear();
    m_currentTime->clear();
    m_additionalCost->clear();

    delete m_currentLoad;
    delete m_distance;
    delete m_currentTankCapacity;
    delete m_currentTime;
    delete m_additionalCost;
}

float CECVRPTW::CalculateRefuelTime(float tankCapacity, float currentTankCapacity) {
    return (tankCapacity - currentTankCapacity) / this->m_ECVRPTWTemplate.GetRefuelingRate();
}

std::vector<int>* CECVRPTW::GetRealPath(AIndividual& individual) {
    std::vector<int>** genotype = new std::vector<int>*();
    this->Evaluate(individual, genotype);
    std::vector<int>* pointerToData = *genotype;
    delete genotype;
    return pointerToData;
}

void CECVRPTW::Evaluate(AIndividual& individual) 
{
    this->Evaluate(individual, nullptr);
}

void CECVRPTW::PrepareData(AIndividual& individual) 
{
    auto& originalGenotype = individual.m_Genotype.m_IntGenotype;
    m_genotypeCpy->clear();
    std::copy(originalGenotype.begin(), originalGenotype.end(), std::back_inserter(*m_genotypeCpy));

    m_genotypeCpy->emplace(m_genotypeCpy->begin(), 0);
    m_genotypeCpy->emplace(m_genotypeCpy->end(), 0);
    for (size_t i = 0; i < m_genotypeCpy->size(); ++i) {
        if ((*m_genotypeCpy)[i] == VEHICLE_DELIMITER) {
            m_genotypeCpy->emplace(m_genotypeCpy->begin() + i, 0);
            m_genotypeCpy->emplace(m_genotypeCpy->begin() + i + 2, 0);
            i += 2;
        }
    }

    std::fill(m_currentLoad->begin(), m_currentLoad->end(), m_ECVRPTWTemplate.GetCapacity());
    std::fill(m_distance->begin(), m_distance->end(), 0);
    std::fill(m_currentTankCapacity->begin(), m_currentTankCapacity->end(), m_ECVRPTWTemplate.GetTankCapcity());
    std::fill(m_currentTime->begin(), m_currentTime->end(), 0);
    std::fill(m_additionalCost->begin(), m_additionalCost->end(), 0);
}

void CECVRPTW::MoveToDepoAndThenToCity(AIndividual& individual,
    size_t& currentIdx,
    int& rechargeStationVisitInSeries,
    size_t& cityIdx,
    size_t& nextCityIdx,
    int& currentCar,
    size_t& depotIdx
)
{
    auto& distMtx = m_ECVRPTWTemplate.GetDistInfoMtx();
    auto& cities = m_ECVRPTWTemplate.GetCities();
    rechargeStationVisitInSeries = 0;
    //To depot
    (*m_distance)[currentCar] += distMtx[cityIdx][depotIdx].m_distance;
    (*m_currentTime)[currentCar] += distMtx[cityIdx][depotIdx].m_travelTime;
    (*m_currentTankCapacity)[currentCar] -= distMtx[cityIdx][depotIdx].m_fuelConsumption;

    //Depot refuel
    (*m_currentTime)[currentCar] += CalculateRefuelTime(m_ECVRPTWTemplate.GetTankCapcity(), (*m_currentTankCapacity)[currentCar]);
    (*m_currentTankCapacity)[currentCar] = m_ECVRPTWTemplate.GetTankCapcity();

    //Depot car loading
    (*m_currentLoad)[currentCar] = m_ECVRPTWTemplate.GetCapacity();

    //To next city
    (*m_distance)[currentCar] += distMtx[depotIdx][nextCityIdx].m_distance;
    (*m_currentTime)[currentCar] += distMtx[depotIdx][nextCityIdx].m_travelTime;
    (*m_currentTankCapacity)[currentCar] -= distMtx[depotIdx][nextCityIdx].m_fuelConsumption;

    (*m_currentLoad)[currentCar] -= cities[nextCityIdx].m_demand;

    //Add depot visit to genotype
    m_genotypeCpy->emplace(m_genotypeCpy->begin() + currentIdx + 1, depotIdx);

    HandleTimeOnCity(individual, currentCar, nextCityIdx);
    currentIdx++;
    cityIdx = nextCityIdx;
}

void CECVRPTW::MoveToNextCity(AIndividual& individual,
    int& rechargeStationVisitInSeries,
    size_t& cityIdx,
    size_t& nextCityIdx,
    int& currentCar,
    size_t& depotIdx
)
{
    auto& distMtx = m_ECVRPTWTemplate.GetDistInfoMtx();
    auto& cities = m_ECVRPTWTemplate.GetCities();

    rechargeStationVisitInSeries = 0;
    (*m_distance)[currentCar] += distMtx[cityIdx][nextCityIdx].m_distance;
    (*m_currentTime)[currentCar] += distMtx[cityIdx][nextCityIdx].m_travelTime;
    (*m_currentTankCapacity)[currentCar] -= distMtx[cityIdx][nextCityIdx].m_fuelConsumption;

    (*m_currentLoad)[currentCar] -= cities[nextCityIdx].m_demand;

    HandleTimeOnCity(individual, currentCar, nextCityIdx);

    (*m_currentTime)[currentCar] += cities[nextCityIdx].m_serviceTime;
    cityIdx = nextCityIdx;
}

void CECVRPTW::HandleTimeOnCity(AIndividual& individual, int& currentCar, size_t& nextCityIdx)
{
    auto& cities = m_ECVRPTWTemplate.GetCities();
    auto dayLength = m_ECVRPTWTemplate.GetMaxDueTime();

    if (std::fmod((*m_currentTime)[currentCar], dayLength) < cities[nextCityIdx].m_readyTime) {
        (*m_currentTime)[currentCar] += cities[nextCityIdx].m_readyTime - std::fmod((*m_currentTime)[currentCar], dayLength);
    }
    else if (std::fmod((*m_currentTime)[currentCar], dayLength) > cities[nextCityIdx].m_dueTime) {
        float timeToEndOfDay = dayLength - std::fmod((*m_currentTime)[currentCar], dayLength);
        (*m_currentTime)[currentCar] += timeToEndOfDay;
        (*m_currentTime)[currentCar] += cities[nextCityIdx].m_readyTime;
        //(*m_additionalCost)[currentCar] += powf(std::fmod((*m_currentTime)[currentCar], dayLength) - cities[nextCityIdx].m_dueTime, 1.2);
        //individual.m_isValid = false;
    }
}

void CECVRPTW::MoveToRechargeStation(size_t& currentIdx, size_t& cityIdx, int& currentCar)
{
    auto& distMtx = m_ECVRPTWTemplate.GetDistInfoMtx();

    size_t nearestChargingStationIdx = GetNearestChargingStationIdx(cityIdx);
    (*m_distance)[currentCar] += distMtx[cityIdx][nearestChargingStationIdx].m_distance;
    (*m_currentTime)[currentCar] += distMtx[cityIdx][nearestChargingStationIdx].m_travelTime;
    (*m_currentTime)[currentCar] += CalculateRefuelTime(m_ECVRPTWTemplate.GetTankCapcity(), (*m_currentTankCapacity)[currentCar]);
    (*m_currentTankCapacity)[currentCar] = m_ECVRPTWTemplate.GetTankCapcity();
    m_genotypeCpy->emplace(m_genotypeCpy->begin() + currentIdx + 1, nearestChargingStationIdx);
    cityIdx = nearestChargingStationIdx;
    if (cityIdx == 0) {
        (*m_currentLoad)[currentCar] = m_ECVRPTWTemplate.GetCapacity();
    }
}

void CECVRPTW::Evaluate(AIndividual& individual, std::vector<int>** genotypeCopy) 
{
    // Build solution
    auto& distMtx = m_ECVRPTWTemplate.GetDistInfoMtx();
    auto& cities = m_ECVRPTWTemplate.GetCities();

    int vehicleCount = m_ECVRPTWTemplate.GetVehicleCount();

    // Evaluate
    int currentCar = 0;
    int rechargeStationVisitInSeries = 0;
    bool isValid = true;
   
    PrepareData(individual);
    
    size_t cityIdx = (*m_genotypeCpy)[0];
    for (size_t i = 0; i < m_genotypeCpy->size(); ++i) {
        size_t nextCityIdx = (*m_genotypeCpy)[(i + 1) % m_genotypeCpy->size()];

        if (nextCityIdx == VEHICLE_DELIMITER) {
            rechargeStationVisitInSeries = 0;
            currentCar++;
            continue;
        }
        size_t nearestChargingStationNearNextCityIdx = GetNearestChargingStationIdx(nextCityIdx);
        size_t depotIdx = GetNearestDepotIdx(cityIdx);

        if ((*m_currentLoad)[currentCar] < cities[nextCityIdx].m_demand && (distMtx[cityIdx][depotIdx].m_fuelConsumption <= (*m_currentTankCapacity)[currentCar] || !isValid)) {
            MoveToDepoAndThenToCity(individual, i, rechargeStationVisitInSeries, cityIdx, nextCityIdx, currentCar, depotIdx);
            if (!isValid) {
                (*m_additionalCost)[currentCar] += 1;
            }
        }
        else if ((*m_currentLoad)[currentCar] >= cities[nextCityIdx].m_demand 
            && (distMtx[cityIdx][nextCityIdx].m_fuelConsumption + distMtx[nextCityIdx][nearestChargingStationNearNextCityIdx].m_fuelConsumption <= (*m_currentTankCapacity)[currentCar] || !isValid)) {
            MoveToNextCity(individual, rechargeStationVisitInSeries, cityIdx, nextCityIdx, currentCar, depotIdx);
            if (!isValid) {
                (*m_additionalCost)[currentCar] += 1;
            }
        }
        else if (++rechargeStationVisitInSeries > 2) {
            isValid = false;
            //individual.m_isValid = false;
            (*m_additionalCost)[currentCar] += 2;
        }
        else {
            MoveToRechargeStation(i, cityIdx, currentCar);
        }
    }


    individual.m_Evaluation[0] = 0;
    individual.m_Evaluation[1] = 0;
    individual.m_Evaluation[2] = 0;
    for (int i = 0; i < vehicleCount; i++) {
        individual.m_Evaluation[0] += (*m_distance)[i];              
        individual.m_Evaluation[1] += (*m_currentTime)[i];
        individual.m_Evaluation[2] += (*m_additionalCost)[i];
    }

    // Normalize
    for (int i = 0; i < 3; i++)
    {
        individual.m_NormalizedEvaluation[i] = (individual.m_Evaluation[i] - m_MinObjectiveValues[i]) / (m_MaxObjectiveValues[i] - m_MinObjectiveValues[i]);
    }

    if (genotypeCopy != nullptr) {
        *genotypeCopy = new std::vector<int>(*m_genotypeCpy);
    }
}

size_t CECVRPTW::GetNearestDepotIdx(const size_t cityIdx) {
    float minDist = FLT_MAX;
    size_t chosenIdx;
    auto& distMtx = m_ECVRPTWTemplate.GetDistInfoMtx();
    auto& depotIndexes = m_ECVRPTWTemplate.GetDepots();
    auto& cities = m_ECVRPTWTemplate.GetCities();

    for (const auto idx : depotIndexes) {
        int depot_index;
        for (int i = 0; i < cities.size(); i++) {
            if (cities[i].m_ID == idx) {
                depot_index = i;
                break;
            }
        }
        if (distMtx[cityIdx][depot_index].m_distance < minDist) {
            chosenIdx = depot_index;
            minDist = distMtx[cityIdx][depot_index].m_distance;
        }
    }
    return chosenIdx;
}

size_t CECVRPTW::GetNearestChargingStationIdx(const size_t cityIdx) {
    float minDist = FLT_MAX;
    size_t chosenIdx;
    auto& distMtx = m_ECVRPTWTemplate.GetDistInfoMtx();
    auto& depotIndexes = m_ECVRPTWTemplate.GetChargingStations();
    auto& cities = m_ECVRPTWTemplate.GetCities();

    for (const auto idx : depotIndexes) {
        int depot_index;
        for (int i = 0; i < cities.size(); i++) {
            if (cities[i].m_ID == idx) {
                depot_index = i;
                break;
            }
        }
        if (distMtx[cityIdx][depot_index].m_distance < minDist) {
            chosenIdx = depot_index;
            minDist = distMtx[cityIdx][depot_index].m_distance;
        }
    }
    return chosenIdx;
}

void CECVRPTW::CreateProblemEncoding() {
    auto& customers = m_ECVRPTWTemplate.GetCustomers();

    SEncodingSection citiesSection = SEncodingSection
    {
        std::vector<SEncodingDescriptor>(customers.size() + m_ECVRPTWTemplate.GetVehicleCount() - 1,
            SEncodingDescriptor{
                    (float)customers[0], (float)customers[customers.size()-1]
            }
        ),
        EEncodingType::PERMUTATION
    };

    m_ProblemEncoding = SProblemEncoding{3, {citiesSection} };
}

void CECVRPTW::LogSolution(AIndividual& individual) {
    auto* genotype = GetRealPath(individual);
    std::string solution;
    for (int i = 0; i < genotype->size(); i++) {
        solution += std::to_string((*genotype)[i]);
        if (i != genotype->size() - 1) {
            solution += ";";
        }
    }
    CExperimentLogger::AddLine(solution.c_str());
    delete genotype;
}

void CECVRPTW::LogAdditionalData() {
    std::ostringstream pointsData;
    auto& cityData = m_ECVRPTWTemplate.GetCities();
    for (auto& city : cityData) {
        
        pointsData << city.m_PosX << ';' << city.m_PosY << ';' << (char)city.m_type << std::endl;
    }
    CExperimentLogger::LogResult(pointsData.str().c_str(), "points.csv");
}

