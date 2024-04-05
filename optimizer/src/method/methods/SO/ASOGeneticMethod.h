#pragma once

#include "../../individual/MO/SMOIndividual.h"
#include "../../AGeneticMethod.h"

class ASOGeneticMethod : public AGeneticMethod
{
public:
    explicit ASOGeneticMethod(AProblem& evaluator, AInitialization& initialization,
        ACrossover& crossover,
        AMutation& mutation, std::vector<float> &objectiveWeights) : AGeneticMethod(evaluator, initialization, crossover, mutation), m_ObjectiveWeights(objectiveWeights)
    {};
    ~ASOGeneticMethod() override = default;

    void Reset()
    {
        for (auto& i : m_Population)
        {
            delete i;
        }
        m_Population.clear();
    };
protected:
    std::vector<float> &m_ObjectiveWeights;
    std::vector<SSOIndividual*> m_Population;
};
