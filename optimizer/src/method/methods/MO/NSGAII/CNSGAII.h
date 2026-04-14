#pragma once

#include <vector>
#include "../../../configMap/SConfigMap.h"
#include "../../../individual/MO/SMOIndividual.h"
#include "../../../operators/selection/selections/CRankedTournament.h"
#include "method/methods/MO/AMOMethod.h"

class CNSGAII : public AMOMethod
{
public:
    CNSGAII(
            AProblem* evaluator,
            AInitialization* initialization,
            CRankedTournament* rankedTournament,
            ACrossover* crossover,
            AMutation* mutation,
            SConfigMap* configMap
    );
    
    ~CNSGAII() {
        delete m_Problem;
        delete m_Initialization;
        delete m_RankedTournament;
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
    ACrossover* m_Crossover;
    AMutation* m_Mutation;

    std::vector<SMOIndividual*> m_Population;
    std::vector<SMOIndividual*> m_NextPopulation;
    std::vector<SMOIndividual*> m_Archive;

    size_t m_PopulationSize = 0;
    size_t m_GenerationLimit = 0;

    void EvolveToNextGeneration();
    void CalcCrowdingDistance(std::vector<SMOIndividual *> &population, std::vector<size_t> &indices);
};