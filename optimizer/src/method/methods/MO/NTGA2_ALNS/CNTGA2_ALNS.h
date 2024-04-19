#pragma once

#include <map>
#include "../AMOGeneticMethod.h"
#include "../../../configMap/SConfigMap.h"
#include "../../../operators/selection/selections/CRankedTournament.h"
#include "../../../operators/selection/selections/CGapSelectionByRandomDim.h"

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
        std::vector<AMutation*>& alnsRemovalMutations,
        std::vector<AMutation*>& alnsInsertionMutations
    );
    ~CNTGA2_ALNS() override = default;

    void RunOptimization() override;
private:
    int m_GapSelectionPercent = 0;
    int m_ALNSProbabilityPercent = 0;
    float m_effectivnessThreshold = 0;
    int m_ALNSIterations = 0;
    int m_ALNSNoImprovementIterations = 0;
    float m_ALNSStartTemperature = 0;
    float m_ALNSTemperatureAnnealingRate = 0;
    int m_ALNSProbabilityStepsIterations = 0;

    std::vector<SMOIndividual*> m_PreviousPopulation;
    std::vector<AMutation*>& m_alnsRemovalMutations;
    std::vector<AMutation*>& m_alnsInsertionMutations;
    CRankedTournament &m_RankedTournament;
    CGapSelectionByRandomDim &m_GapSelection;
    
    void CrossoverAndMutate(SMOIndividual &firstParent, SMOIndividual &secondParent);
    void EvaluateAndAdd(SMOIndividual& individual);
    bool ShouldUseALNS(std::vector<SMOIndividual*>& previousPopulation, std::vector<SMOIndividual*> currentPopulation);
    bool AcceptWorseSolution(SMOIndividual& generated, SMOIndividual& current, float temperature);

    void RunGenerationWithGap();
    void RunGeneration();

    void LogResult();

    void UpdateProbabilityTables(std::map<AMutation*, std::tuple<float, int>>& removalOperatorsScores,
        std::vector<float>& removalOperatorsProbabilityDistribution,
        std::map<AMutation*, std::tuple<float, int>>& insertOperatorsScores,
        std::vector<float>& insertionOperatorsProbabilityDistribution
    );

    void UpdateScores(SMOIndividual* current,
        SMOIndividual* best,
        AMutation*& removalOperator,
        AMutation*& insertOperator,
        std::map<AMutation*, std::tuple<float, int>>& removalOperatorsScores,
        std::map<AMutation*, std::tuple<float, int>>& insertOperatorsScores
    );

    SMOIndividual* RunALNS(SMOIndividual& parent);
};