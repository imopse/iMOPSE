#pragma once

#include "../AMOGeneticMethod.h"
#include "../../../configMap/SConfigMap.h"
#include "../../../operators/selection/selections/CRankedTournament.h"
#include "../../../operators/selection/selections/CGapSelectionByRandomDim.h"

class CNTGA2 : public AMOGeneticMethod
{
public:
    CNTGA2(
            AProblem &evaluator,
            AInitialization &initialization,
            CRankedTournament &rankedTournament,
            CGapSelectionByRandomDim &gapSelection,
            ACrossover &crossover,
            AMutation &mutation,
            SConfigMap *configMap
    );
    ~CNTGA2() override = default;

    void RunOptimization() override;
private:
    int m_GapSelectionPercent = 0;

    CRankedTournament &m_RankedTournament;
    CGapSelectionByRandomDim &m_GapSelection;
    
    void CrossoverAndMutate(SMOIndividual &firstParent, SMOIndividual &secondParent);
};