#pragma once

#include "individual/CGPHHIndividual.h"
#include "../../../configMap/SConfigMap.h"
#include "SGPHHLogConfig.h"
#include "../../../operators/selection/selections/CFitnessTournament.h"
#include "constructive/IGPHHConstructive.h"
#include "method/methods/SO/ASOMethod.h"

class CGPHH_ECVRP : public ASOMethod
{
public:
    CGPHH_ECVRP(
        AProblem* evaluator,
        AInitialization* initialization,
        CFitnessTournament* fitnessTournament,
        ACrossover* crossover,
        AMutation* mutation,
        IGPHHConstructive* constructive,
        SConfigMap* configMap,
        std::vector<float>* objectiveWeights
    );
    
    ~CGPHH_ECVRP() {
        delete m_Problem;
        delete m_Initialization;
        delete m_FitnessTournament;
        delete m_Crossover;
        delete m_Mutation;
        delete m_ObjectiveWeights;
        delete m_Constructive;
    };

    void RunOptimization() override;

    void Reset() override {
        for (auto &i: m_Population)
        {
            delete i;
        }
        m_Population.clear();
    }
private:
    AProblem* m_Problem;
    AInitialization* m_Initialization;
    CFitnessTournament* m_FitnessTournament;
    ACrossover* m_Crossover;
    AMutation* m_Mutation;
    IGPHHConstructive* m_Constructive;
    std::vector<float>* m_ObjectiveWeights;
    
    std::vector<SSOIndividual*> m_Population;
    
    int m_PopulationSize = 0;
    int m_GenerationLimit = 0;
    int m_EliteSize;
    
    SGPHHLogConfig m_LogConfig;

    void CreateIndividual();
    void EvolveToNextGeneration();
};
