#pragma once

#include "../../AMethod.h"
#include "../../individual/MO/SMOIndividual.h"
#include "../../AGeneticMethod.h"

class AMOGeneticMethod : public AGeneticMethod
{
public:
    explicit AMOGeneticMethod(AProblem& evaluator, AInitialization& initialization,
        ACrossover& crossover,
        AMutation& mutation) : AGeneticMethod(evaluator, initialization, crossover, mutation)
    {};
    ~AMOGeneticMethod() override = default;

    void Reset() override
    {
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
    };
protected:
    std::vector<SMOIndividual*> m_Population;
    std::vector<SMOIndividual*> m_NextPopulation;
    std::vector<SMOIndividual*> m_Archive;
};
