#pragma once

#include "../ASOGeneticMethod.h"
#include "individual/CGPHHIndividual.h"
#include "../../../configMap/SConfigMap.h"
#include "SGPHHLogConfig.h"
#include "../../../operators/selection/selections/CFitnessTournament.h"
#include "constructive/IGPHHConstructive.h"

class CGPHH_ECVRP : public ASOGeneticMethod {
public:
    CGPHH_ECVRP(
        std::vector<float> &objectiveWeights,
        AProblem &evaluator,
        AInitialization &initialization,
        CFitnessTournament &fitnessTournament,
        ACrossover &crossover,
        AMutation &mutation,
        IGPHHConstructive* constructive,
        SConfigMap *configMap
    );
    
    void RunOptimization() override;

private:
    void CreateIndividual();
    void EvolveToNextGeneration();
    
    CFitnessTournament& m_FitnessTournament;
    int m_PopulationSize;
    int m_GenerationLimit;
    int m_EliteSize;
    
    SGPHHLogConfig m_LogConfig;
    IGPHHConstructive* m_Constructive;
};
