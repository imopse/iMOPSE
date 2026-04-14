#pragma once

#include "../../../configMap/SConfigMap.h"
#include "../../../operators/selection/selections/CRankedTournament.h"
#include "../../../operators/selection/selections/CGapSelectionByRandomDim.h"
#include "method/methods/MO/AMOMethod.h"

class CNTGA2 : public AMOMethod
{
public:
    CNTGA2(
            AProblem* evaluator,
            AInitialization* initialization,
            CRankedTournament* rankedTournament,
            CGapSelectionByRandomDim* gapSelection,
            ACrossover* crossover,
            AMutation* mutation,
            SConfigMap* configMap
    );
    
    ~CNTGA2() {
        delete m_Problem;
        delete m_Initialization;
        delete m_RankedTournament;
        delete m_GapSelection;
        delete m_Crossover;
        delete m_Mutation;
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
    }
private:
    AProblem* m_Problem;
    AInitialization* m_Initialization;
    CRankedTournament* m_RankedTournament;
    CGapSelectionByRandomDim* m_GapSelection;
    ACrossover* m_Crossover;
    AMutation* m_Mutation;
    
    std::vector<SMOIndividual*> m_Population;
    std::vector<SMOIndividual*> m_NextPopulation;
    std::vector<SMOIndividual*> m_Archive;
    
    size_t m_PopulationSize = 0;
    size_t m_GenerationLimit = 0;
    int m_GapSelectionPercent = 0;
    
    void CrossoverAndMutate(SMOIndividual &firstParent, SMOIndividual &secondParent);
};