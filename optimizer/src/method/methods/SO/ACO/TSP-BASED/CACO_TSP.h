#pragma once

#include "method/configMap/SConfigMap.h"
#include "method/methods/SO/ASOMethod.h"

enum InitType {
    Uniform = 1,
    Distance = 2
};

class CACO_TSP : public ASOMethod {
public:
    CACO_TSP( AProblem* evaluator, AInitialization* initialization, SConfigMap* configMap, std::vector<float>* objectiveWeights );

    ~CACO_TSP() {
        delete m_Problem;
        delete m_Initialization;
        delete m_GloballyBest;
        delete m_ObjectiveWeights;
    };

    void RunOptimization() override;

    void Reset() override;
private:
    AProblem* m_Problem;
    AInitialization* m_Initialization;
    std::vector<float>* m_ObjectiveWeights;
    SSOIndividual *m_GloballyBest;
    
    std::vector<SSOIndividual*> m_Population;
    std::vector<std::vector<float>> m_PheromoneMap;
    float m_ReducingMultiplier;
    int m_GenerationLimit;
    int m_PopulationSize;
    InitType m_InitType;
    std::vector<std::vector<float>> m_DistanceMatrix;
    
    void AddExperimentData(int generation);
    void RunAnts();
    void AntMarch();
    void LeavePheromone();
    void RandomAnts();
    void GetBestRoute();
    SSOIndividual *GetRandomAnt();
    void SavePheromoneMap(int generation);
};

