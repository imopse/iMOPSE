#pragma once

#include <chrono>
#include <deque>
#include "method/configMap/SConfigMap.h"
#include "method/individual/MO/SMOIndividual.h"
#include "method/operators/selection/selections/CRankedTournament.h"
#include "method/operators/selection/selections/CGapSelectionByRandomDim.h"
#include "method/multiOperator/CAdaptiveOperatorManager.h"
#include "method/methods/MO/AMOMethod.h"

class CFANGA : public AMOMethod
{
public:
    CFANGA(
        AProblem* evaluator,
        AInitialization* initialization,
        ACrossover* crossover,
        AMutation* mutation,
        CGapSelectionByRandomDim* gapSelection,
        SConfigMap* configMap
    );

    ~CFANGA() {
        delete m_Problem;
        delete m_Initialization;
        delete m_GapSelection;
        delete m_Crossover;
        delete m_Mutation;
        delete m_MultiMutation;
    };

    void RunOptimization() override;

    void Reset() override {
        for (auto& i : m_Population)
        {
            delete i;
        }
        m_Population.clear();
        for (auto &i: m_NextPopulation)
        {
            delete i;
        }
        m_NextPopulation.clear();
        for (auto &i: m_Archive)
        {
            delete i;
        }
        m_Archive.clear();

        m_Generation = 0;
        m_MultiMutation->ResetAllOperatorData();
    }

private:
    AProblem* m_Problem;
    AInitialization* m_Initialization;
    CGapSelectionByRandomDim* m_GapSelection;
    ACrossover* m_Crossover;
    AMutation* m_Mutation;
    AMultiOperator<AMutation>* m_MultiMutation = nullptr;

    std::vector<SMOIndividual*> m_Population;
    std::vector<SMOIndividual*> m_NextPopulation;
    std::vector<SMOIndividual*> m_Archive;

    int m_Generation = 0;
    size_t m_PopulationSize = 0;
    size_t m_GenerationLimit = 0;
    
    CAdaptiveOperatorManager m_AdaptiveOperatorManager;

    void EvolveToNextGeneration();
    void CrossoverAndMutate(SMOIndividual* firstParent, SMOIndividual* secondParent);
    void LogIndividualsToCSV(CCSV<float>& csv, const std::vector<SMOIndividual*>& individuals) const;
    void LogHVStatsToCSV(CCSV<float>& csv);

    // TODO - copied from ParetoAnalyzer - remove it or move somewhere else
    // Calculate Hyper-volume using values as they are (either absolute or normalized), using the reference point
    float CalcHV(std::vector<SMOIndividual*>& individuals, const std::vector<float>& refPoint);

    std::chrono::time_point<std::chrono::steady_clock> m_StartTime;
};