#pragma once

#include <vector>
#include <cstddef>
#include <cstddef>

class CECVRPTWTemplate;

class CECVRPTWSolution
{
public:
    CECVRPTWSolution(CECVRPTWTemplate* problemTemplate);

    [[nodiscard]] float GetTotalDistance() const;
    [[nodiscard]] float GetTotalDuration() const;

    void BuildSolution(const std::vector<int>& initialAssignment);
    [[nodiscard]] const std::vector<int>& GetSolution() const { return m_Solution; }

private:

    void PrepareData(const std::vector<int>& initialAssignment);
    [[nodiscard]] bool CanSatisfyDemand(std::size_t carIdx, std::size_t cityIdx) const;
    [[nodiscard]] bool CanSafelyReach(std::size_t carIdx, std::size_t cityIdx) const;
    float CalculateRefuelTime(float tankCapacity, float currentTankCapacity);
    void MoveCarToDepoLoadAndRecharge(std::size_t carIdx, std::size_t depotIdx);
    void MoveCarToDepoLoadRechargeAndThenToCity(std::size_t carIdx, std::size_t depotIdx, std::size_t nextCityIdx);
    void MoveCarToNextCity(std::size_t carIdx, std::size_t nextCityIdx);
    void HandleTimeOnCity(std::size_t carIdx, std::size_t nextCityIdx);
    void MoveCarToNearestRechargeStation(std::size_t carIdx);
    void MoveCarToRechargeStationTowardsCity(std::size_t carIdx, std::size_t nextCityIdx);
    void MoveCarToRechargeStation(std::size_t carIdx, std::size_t stationIdx);

    CECVRPTWTemplate* m_ECVRPTWTemplate;

    std::vector<int> m_CurrentLoad;
    std::vector<std::size_t> m_CurrentPosition;
    std::vector<float> m_Distance;
    std::vector<float> m_CurrentTankCapacity;
    std::vector<float> m_CurrentTime;

    std::vector<int> m_Solution;
    std::size_t m_CurrentSolutionIdx;
};

