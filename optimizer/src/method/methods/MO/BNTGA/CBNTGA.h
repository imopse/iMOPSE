#pragma once

#include <chrono>
#include "../../../configMap/SConfigMap.h"
#include "../../../individual/MO/SMOIndividual.h"
#include "../../../operators/selection/selections/CRankedTournament.h"
#include "../../../operators/selection/selections/CGapSelectionByRandomDim.h"
#include "method/methods/MO/AMOMethod.h"

class CBNTGA : public AMOMethod
{
public:
    CBNTGA(
            AProblem* evaluator,
            AInitialization* initialization,
            ACrossover* crossover,
            AMutation* mutation,
            CGapSelectionByRandomDim* gapSelection,
            SConfigMap* configMap
    );
    
    ~CBNTGA() {
        delete m_Problem;
        delete m_Initialization;
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

        m_Generation = 0;
    }

private:
    AProblem* m_Problem;
    AInitialization* m_Initialization;
    CGapSelectionByRandomDim* m_GapSelection;
    ACrossover* m_Crossover;
    AMutation* m_Mutation;

    std::vector<SMOIndividual*> m_Population;
    std::vector<SMOIndividual*> m_NextPopulation;
    std::vector<SMOIndividual*> m_Archive;

    int m_Generation = 0;
    size_t m_PopulationSize = 0;
    size_t m_GenerationLimit = 0;
    
    void EvolveToNextGeneration();
    void CrossoverAndMutate(SMOIndividual *firstParent, SMOIndividual *secondParent);

    // TODO - copied from ParetoAnalyzer - remove it or move somewhere else
    // Calculate Hyper-volume using values as they are (either absolute or normalized), using the reference point
    float CalcHV(std::vector<SMOIndividual*>& individuals, const std::vector<float>& refPoint);

    std::chrono::time_point<std::chrono::steady_clock> m_StartTime;
};
