#pragma once

#include <map>
#include "../AMOGeneticMethod.h"
#include "../../../configMap/SConfigMap.h"
#include "../../../operators/selection/selections/CRankedTournament.h"
#include "../../../operators/selection/selections/CGapSelectionByRandomDim.h"
#include "../../SO/ALNS/CALNS.h"

class CNTGA2_ALNS : public AMOGeneticMethod
{
public:
    CNTGA2_ALNS(
        AProblem& evaluator,
        AInitialization& initialization,
        CRankedTournament& rankedTournament,
        CGapSelectionByRandomDim& gapSelection,
        ACrossover& crossover,
        AMutation& mutation,
        SConfigMap* configMap,
        std::vector<CALNS*>& alnsInstances
    );
    ~CNTGA2_ALNS() override = default;

    void RunOptimization() override;
private:
    int m_GapSelectionPercent = 0;
    float m_ALNSProbabilityPercent = 0;
    float m_effectivnessThreshold = 0;

    std::vector<SMOIndividual*> m_PreviousPopulation;
    std::vector<CALNS*>& m_ALNSInstances;
    CRankedTournament &m_RankedTournament;
    CGapSelectionByRandomDim &m_GapSelection;
    
    void CrossoverAndMutate(SMOIndividual &firstParent, SMOIndividual &secondParent);
    void EvaluateAndAdd(SMOIndividual& individual);
    bool ShouldUseALNS(std::vector<SMOIndividual*>& previousPopulation, std::vector<SMOIndividual*> currentPopulation);

    SMOIndividual* RunALNS(SMOIndividual& parent);

    void RunGenerationWithGap();
    void RunGeneration();

    void LogResult();
};