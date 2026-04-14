#pragma once

#include <map>
#include "../../../configMap/SConfigMap.h"
#include "../../../operators/selection/selections/CRankedTournament.h"
#include "../../../operators/selection/selections/CGapSelectionByRandomDim.h"
#include "method/methods/MO/AMOMethod.h"

class CNTGA2_ALNS : public AMOMethod
{
public:
    CNTGA2_ALNS(
        AProblem* evaluator,
        AInitialization* initialization,
        CRankedTournament* rankedTournament,
        CGapSelectionByRandomDim* gapSelection,
        ACrossover* crossover,
        AMutation* mutation,
        SConfigMap* configMap,
        std::vector<AMutation*>* alnsRemovalMutations,
        std::vector<AMutation*>* alnsInsertionMutations
    );
    
    ~CNTGA2_ALNS() {
        delete m_Problem;
        delete m_Initialization;
        delete m_GapSelection;
        delete m_Crossover;
        delete m_Mutation;
        delete m_RankedTournament;
        
        for (auto* mutation: *m_alnsRemovalMutations) {
            delete mutation;
        }
        delete m_alnsRemovalMutations;
        for (auto* mutation: *m_alnsInsertionMutations) {
            delete mutation;
        }
        delete m_alnsInsertionMutations;
    };

    void RunOptimization() override;

    void Reset() override {
        for (auto& i : m_Population)
        {
            delete i;
        }
        m_Population.clear();
        m_PreviousPopulation.clear();
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
    }
private:
    AProblem* m_Problem;
    AInitialization* m_Initialization;
    CGapSelectionByRandomDim* m_GapSelection;
    ACrossover* m_Crossover;
    AMutation* m_Mutation;
    std::vector<AMutation*>* m_alnsRemovalMutations;
    std::vector<AMutation*>* m_alnsInsertionMutations;
    CRankedTournament* m_RankedTournament;
    
    std::vector<SMOIndividual*> m_PreviousPopulation;
    std::vector<SMOIndividual*> m_Population;
    std::vector<SMOIndividual*> m_NextPopulation;
    std::vector<SMOIndividual*> m_Archive;

    int m_Generation = 0;
    size_t m_PopulationSize = 0;
    size_t m_GenerationLimit = 0;
    
    int m_GapSelectionPercent = 0;
    int m_ALNSProbabilityPercent = 0;
    float m_effectivnessThreshold = 0;
    int m_ALNSIterations = 0;
    int m_ALNSNoImprovementIterations = 0;
    float m_ALNSStartTemperature = 0;
    float m_ALNSTemperatureAnnealingRate = 0;
    int m_ALNSProbabilityStepsIterations = 0;
    
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