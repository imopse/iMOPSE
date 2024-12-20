#pragma once

#include <vector>
#include <cstddef>

class CECVRPTWTemplate;

class CECVRPTWSolution
{
public:
    CECVRPTWSolution(CECVRPTWTemplate& problemTemplate);

    [[nodiscard]] float GetTotalDistance() const;
    [[nodiscard]] float GetTotalDuration() const;

    void BuildSolution(const std::vector<int>& initialAssignment);
    [[nodiscard]] const std::vector<int>& GetSolution() const { return m_Solution; }

private:

    void PrepareData(const std::vector<int>& initialAssignment);
    [[nodiscard]] bool CanSatisfyDemand(size_t carIdx, size_t cityIdx) const;
    [[nodiscard]] bool CanSafelyReach(size_t carIdx, size_t cityIdx) const;
    float CalculateRefuelTime(float tankCapacity, float currentTankCapacity);
    void MoveCarToDepoLoadAndRecharge(size_t carIdx, size_t depotIdx);
    void MoveCarToDepoLoadRechargeAndThenToCity(size_t carIdx, size_t depotIdx, size_t nextCityIdx);
    void MoveCarToNextCity(size_t carIdx, size_t nextCityIdx);
    void HandleTimeOnCity(size_t carIdx, size_t nextCityIdx);
    void MoveCarToNearestRechargeStation(size_t carIdx);
    void MoveCarToRechargeStationTowardsCity(size_t carIdx, size_t nextCityIdx);
    void MoveCarToRechargeStation(size_t carIdx, size_t stationIdx);

    CECVRPTWTemplate& m_ECVRPTWTemplate;

    std::vector<int> m_CurrentLoad;
    std::vector<size_t> m_CurrentPosition;
    std::vector<float> m_Distance;
    std::vector<float> m_CurrentTankCapacity;
    std::vector<float> m_CurrentTime;

    std::vector<int> m_Solution;
    size_t m_CurrentSolutionIdx;
};

