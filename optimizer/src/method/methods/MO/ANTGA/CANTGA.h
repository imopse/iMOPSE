#pragma once

#include <chrono>
#include <deque>
#include "method/configMap/SConfigMap.h"
#include "method/individual/MO/SMOIndividual.h"
#include "method/operators/selection/selections/CRankedTournament.h"
#include "method/operators/selection/selections/CGapSelectionByRandomDim.h"
#include "method/multiOperator/AMultiOperator.h"
#include "method/multiOperator/CMultiOperatorRegion.h"
#include "method/methods/MO/AMOMethod.h"

template <typename T> class CCSV;

class CANTGA : public AMOMethod
{
public:
    CANTGA(
        AProblem* evaluator,
        AInitialization* initialization,
        ACrossover* crossover,
        AMutation* mutation,
        CGapSelectionByRandomDim* gapSelection,
        SConfigMap* configMap
    );

    ~CANTGA() {
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
    SConfigMap* configMap;
    
    std::vector<SMOIndividual*> m_Population;
    std::vector<SMOIndividual*> m_NextPopulation;
    std::vector<SMOIndividual*> m_Archive;
    
    int m_Generation = 0;
    size_t m_PopulationSize = 0;
    size_t m_GenerationLimit = 0;

    void EvolveToNextGeneration();
    void CrossoverAndMutate(SMOIndividual* firstParent, SMOIndividual* secondParent);
    void LocalAdaptiveMutation(SMOIndividual* individual, SMOIndividual* parent, SMOIndividual* otherParent);
    void GlobalAdaptiveMutation(SMOIndividual* firstChild, SMOIndividual* secondChild, SMOIndividual* firstParent, SMOIndividual* secondParent);
    float CalculateCredit(SMOIndividual* individual, SMOIndividual* parent, SMOIndividual* otherParent);
    void LogIndividualsToCSV(CCSV<float>& csv, const std::vector<SMOIndividual*>& individuals) const;
    void LogOperatorStatsToCSV(CCSV<float>& csv) const;
    void LogHVStatsToCSV(CCSV<float>& csv);

    void ResetAllArchiveOperatorDataButAccCalls();
    void SetMultiMutationCreditBasedOnArchive();
    bool IsNonDominatedByArchive(const SMOIndividual* ind) const;
    bool IsNonDominatedByPopulation(const SMOIndividual* ind) const;
    float NonDominatedByPopFrac(const SMOIndividual* ind) const;
    float NonDominatedByArchFrac(const SMOIndividual* ind) const;
    float CalcDominationScoreByPop(const SMOIndividual* ind) const;
    float CalcDominationScoreByArch(const SMOIndividual* ind) const;
    float CalcDominationScore2ByArch(const SMOIndividual* ind) const;

    float CalcFitnessImprovementRateVer1(SMOIndividual* newIndividual, SMOIndividual* oldIndividual);
    float CalcFitnessImprovementRateVer3(SMOIndividual* newIndividual, SMOIndividual* oldIndividual);
    float CalcFitnessImprovementRateVer4(SMOIndividual* newIndividual, SMOIndividual* oldIndividual);
    float CalcFitnessImprovementRateVer5(SMOIndividual* newIndividual, SMOIndividual* oldIndividual);

    // TODO - copied from ParetoAnalyzer - remove it or move somewhere else
    // Calculate Hyper-volume using values as they are (either absolute or normalized), using the reference point
    float CalcHV(std::vector<SMOIndividual*>& individuals, const std::vector<float>& refPoint);

    std::chrono::time_point<std::chrono::steady_clock> m_StartTime;
};