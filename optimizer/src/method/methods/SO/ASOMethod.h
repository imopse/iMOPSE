#pragma once

#include "../../AMethod.h"
#include "../../individual/SO/SSOIndividual.h"

class ASOMethod : public AMethod
{
public:
    explicit ASOMethod(AProblem &evaluator, AInitialization &initialization, std::vector<float>& objectiveWeights) :
    AMethod(evaluator, initialization), m_ObjectiveWeights(objectiveWeights)
    {};
    ~ASOMethod() override = default;

    void Reset() override
    {
        for (auto &i: m_Population)
        {
            delete i;
        }
        m_Population.clear();
    };
protected:
    std::vector<float> &m_ObjectiveWeights;
    std::vector<SSOIndividual *> m_Population;
};

