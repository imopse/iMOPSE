#pragma once

#include "../../../operators/initialization/AInitialization.h"
#include "../../../operators/crossover/ACrossover.h"
#include "../../../operators/mutation/AMutation.h"
#include "../../../individual/SO/SSOIndividual.h"
#include "../../../configMap/SConfigMap.h"
#include "../../../operators/selection/selections/CFitnessTournament.h"
#include "method/methods/SO/ASOMethod.h"

class CGA : public ASOMethod
{
public:
    CGA(
            AProblem* evaluator,
            AInitialization* initialization,
            CFitnessTournament* fitnessTournament,
            ACrossover* crossover,
            AMutation* mutation,
            SConfigMap* configMap,
            std::vector<float>* objectiveWeights
    );
    ~CGA() {
        delete m_Problem;
        delete m_Initialization;
        delete m_FitnessTournament;
        delete m_Crossover;
        delete m_Mutation;
        delete m_ObjectiveWeights;
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
    std::vector<float>* m_ObjectiveWeights;
    
    std::vector<SSOIndividual*> m_Population;
    
    size_t m_GenerationLimit = 0;
    size_t m_PopulationSize = 0;

    void CreateIndividual();
    void EvolveToNextGeneration();
};