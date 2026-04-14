#pragma once

#include "../../../configMap/SConfigMap.h"
#include "method/methods/SO/ASOMethod.h"

class CDE : public ASOMethod
{
public:
    CDE(AProblem *evaluator, AInitialization *initialization, SConfigMap *configMap, std::vector<float> *objectiveWeights);
    ~CDE() {
        delete m_Problem;
        delete m_Initialization;
        delete m_ObjectiveWeights;
    };

    void RunOptimization() override;

    void Reset() override
    {
        for (auto& i : m_Population)
        {
            delete i;
        }
        m_Population.clear();
    };
private:
    AProblem* m_Problem;
    AInitialization* m_Initialization;
    std::vector<float>* m_ObjectiveWeights;
    
    std::vector<SSOIndividual*> m_Population;
    size_t m_PopulationSize = 0;
    size_t m_GenerationLimit = 0;
    float m_Cr;
    float m_F;

    void CreateIndividual();
    void EvolveToNextGeneration();
    void DifferentialEvolutionStep(SGenotype& donor, const SGenotype& gens1, const SGenotype& gens2, const SGenotype& gens3);
};
