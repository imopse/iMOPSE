#pragma once

#include "../../../configMap/SConfigMap.h"
#include "../ASOMethod.h"

class CDE : public ASOMethod
{
public:
    CDE(
            std::vector<float>& objectiveWeights,
            AProblem& evaluator,
            AInitialization& initialization,
            SConfigMap* configMap
    );
    ~CDE() override = default;

    void RunOptimization() override;

    void Reset()
    {
        for (auto& i : m_Population)
        {
            delete i;
        }
        m_Population.clear();
    };
private:
    size_t m_PopulationSize = 0;
    size_t m_GenerationLimit = 0;
    float m_Cr;
    float m_F;

    void CreateIndividual();
    void EvolveToNextGeneration();
    void DifferentialEvolutionStep(SGenotype& donor, const SGenotype& gens1, const SGenotype& gens2, const SGenotype& gens3);
};
