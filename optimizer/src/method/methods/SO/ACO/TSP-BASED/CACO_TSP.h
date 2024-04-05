#pragma once

#include "../CACO.h"

enum InitType {
    Uniform = 1,
    Distance = 2
};

class CACO_TSP : public CACO {
public:
    CACO_TSP(
        AProblem &evaluator,
        AInitialization &initialization,
        SConfigMap *configMap,
        std::vector<float> &objectiveWeights
    );

    ~CACO_TSP() override = default;

    void RunOptimization() override;

private:
    std::vector<std::vector<float>> m_PheromoneMap;
    float m_ReducingMultiplier;
    SSOIndividual *m_GloballyBest;
    int m_GenerationLimit;
    int m_PopulationSize;
    InitType m_InitType;
    std::vector<std::vector<float>> m_DistanceMatrix;

    void LogResultData();

    void AddExperimentData(int generation);

    void RunAnts();

    void AntMarch();

    void LeavePheromone();

    void RandomAnts();

    void GetBestRoute();

    SSOIndividual *GetRandomAnt();

    void ResetPheromoneMap();

    void SavePheromoneMap(int generation);
};

