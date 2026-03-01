#pragma once

#include <chrono>
#include <deque>
#include "method/configMap/SConfigMap.h"
#include "method/individual/MO/SMOIndividual.h"
#include "method/methods/MO/AMOGeneticMethod.h"
#include "method/operators/selection/selections/CRankedTournament.h"
#include "method/operators/selection/selections/CGapSelectionByRandomDim.h"
#include "method/multiOperator/AMultiOperator.h"
#include "method/multiOperator/CMultiOperatorRegion.h"

template <typename T> class CCSV;

class CANTGA : public AMOGeneticMethod
{
public:
    CANTGA(
        AProblem& evaluator,
        AInitialization& initialization,
        ACrossover& crossover,
        AMutation& mutation,
        CGapSelectionByRandomDim& gapSelection,
        SConfigMap* configMap
    );

    ~CANTGA();

    void RunOptimization() override;
    void Reset() override;

private:
    int m_Generation = 0;
    CGapSelectionByRandomDim& m_GapSelection;
    AMultiOperator<AMutation>* m_MultiMutation = nullptr;

    struct SWindowSlot
    {
        size_t m_OperatorIdx = 0;
        float m_Credits = 0.f;
        float m_Calls = 0.f;
    };

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