#pragma once

#include "CECVRPTWTemplate.h"
#include "../../AProblem.h"
#include "../../../method/individual/SGenotype.h"
#include <iterator>

#define GENOTYPEBUFFERSIZE 100

class CECVRPTW : public AProblem {
public:
    explicit CECVRPTW(CECVRPTWTemplate& cvrpBase);

    ~CECVRPTW() override;

    SProblemEncoding& GetProblemEncoding() override { return this->m_ProblemEncoding; }

    void Evaluate(AIndividual& individual) override;
    void LogSolution(AIndividual& individual) override;
    void LogAdditionalData() override;

    CECVRPTWTemplate& GetECVRPTWTemplate() { return m_ECVRPTWTemplate; }
    std::vector<int>* GetRealPath(AIndividual& individual);
protected:
    size_t GetNearestChargingStationIdx(size_t cityIdx);
    size_t GetNearestDepotIdx(size_t cityIdx);

    std::vector<size_t> m_UpperBounds;
    SProblemEncoding m_ProblemEncoding;
    CECVRPTWTemplate& m_ECVRPTWTemplate;
    std::vector<float> m_MaxObjectiveValues;
    std::vector<float> m_MinObjectiveValues;

private:
    std::vector<int>* m_genotypeCpy;

    std::vector<int>* m_currentLoad;
    std::vector<float>* m_distance;
    std::vector<float>* m_currentTankCapacity;
    std::vector<float>* m_currentTime;
    std::vector<float>* m_additionalCost;

    void Evaluate(AIndividual& individual, std::vector<int>** genotypeCopy);
    void CreateProblemEncoding();
    float CalculateRefuelTime(float tankCapacity, float currentTankCapacity);

    void PrepareData(AIndividual& individual);
    void MoveToDepoAndThenToCity(AIndividual& individual,
        size_t& currentIdx,
        int& rechargeStationVisitInSeries,
        size_t& cityIdx,
        size_t& nextCityIdx,
        int& currentCar,
        size_t& depotIdx
    );

    void MoveToNextCity(AIndividual& individual,
        int& rechargeStationVisitInSeries,
        size_t& cityIdx,
        size_t& nextCityIdx,
        int& currentCar,
        size_t& depotIdx
    );

    void HandleTimeOnCity(AIndividual& individual, int& currentCar, size_t& nextCityIdx);

    void MoveToRechargeStation(size_t& currentIdx, size_t& cityIdx, int& currentCar);
};
