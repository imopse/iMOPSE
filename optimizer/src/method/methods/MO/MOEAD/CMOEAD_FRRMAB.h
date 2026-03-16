#pragma once

#include <chrono>
#include <deque>
#include "CMOEAD.h"
#include "method/multiOperator/AMultiOperator.h"

class CMOEAD_FRRMAB : public CMOEAD
{
public:
    CMOEAD_FRRMAB(
        AProblem &evaluator,
        AInitialization &initialization,
        ACrossover &crossover,
        AMutation &mutation,
        SConfigMap *configMap
    );
    ~CMOEAD_FRRMAB();

    void RunOptimization() override;
    void Reset() override;
private:

    struct SWindowSlot
    {
        size_t m_OperatorIdx = 0;
        float m_OperatorFIR = 0.f;
    };

    AMultiOperator<AMutation>* m_MultiMutation = nullptr;
    std::deque<SWindowSlot> m_SlidingWindow;
    size_t m_SlidingWindowWidth = 0;

    void EvolveToNextGeneration();
    float CalcFitnessImprovementRate(SMOIndividual* newIndividual, SMOIndividual* oldIndividual, CMOEAD::SSubproblem& subproblem);
    void UpdateMultiOperatorCredits();

    // TODO - copied from ParetoAnalyzer - remove it or move somewhere else
    // Calculate Hyper-volume using values as they are (either absolute or normalized), using the reference point
    float CalcHV(std::vector<SMOIndividual*>& individuals, const std::vector<float>& refPoint);

    std::chrono::time_point<std::chrono::steady_clock> m_StartTime;
};