#pragma once

#include <chrono>
#include <deque>
#include "method/configMap/SConfigMap.h"
#include "method/individual/MO/SMOIndividual.h"
#include "method/methods/MO/AMOGeneticMethod.h"
#include "method/operators/selection/selections/CRankedTournament.h"
#include "method/operators/selection/selections/CGapSelectionByRandomDim.h"
#include "method/multiOperator/CAdaptiveOperatorManager.h"

class CFANGA : public AMOGeneticMethod
{
public:
    CFANGA(
        AProblem& evaluator,
        AInitialization& initialization,
        ACrossover& crossover,
        AMutation& mutation,
        CGapSelectionByRandomDim& gapSelection,
        SConfigMap* configMap
    );

    ~CFANGA();

    void RunOptimization() override;
    void Reset() override;

private:
    int m_Generation = 0;
    CGapSelectionByRandomDim& m_GapSelection;
    CAdaptiveOperatorManager m_AdaptiveOperatorManager;

    struct SWindowSlot
    {
        size_t m_OperatorIdx = 0;
        float m_Credits = 0.f;
        float m_Calls = 0.f;
    };

    void EvolveToNextGeneration();
    void CrossoverAndMutate(SMOIndividual* firstParent, SMOIndividual* secondParent);
    void LogIndividualsToCSV(CCSV<float>& csv, const std::vector<SMOIndividual*>& individuals) const;
    void LogHVStatsToCSV(CCSV<float>& csv);

    // TODO - copied from ParetoAnalyzer - remove it or move somewhere else
    // Calculate Hyper-volume using values as they are (either absolute or normalized), using the reference point
    float CalcHV(std::vector<SMOIndividual*>& individuals, const std::vector<float>& refPoint);

    std::chrono::time_point<std::chrono::steady_clock> m_StartTime;
};