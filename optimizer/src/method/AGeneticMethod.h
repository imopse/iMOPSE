#pragma once

#pragma once

#include "operators/initialization/AInitialization.h"
#include "operators/crossover/ACrossover.h"
#include "operators/mutation/AMutation.h"
#include "../problem/AProblem.h"
#include "AMethod.h"

class AGeneticMethod : public AMethod
{
public:
    explicit AGeneticMethod(AProblem& evaluator, AInitialization& initialization,
        ACrossover& crossover,
        AMutation& mutation) : m_Crossover(crossover), m_Mutation(mutation), AMethod(evaluator, initialization)
    {};
    virtual ~AGeneticMethod() = default;
    
protected:
    size_t m_GenerationLimit = 0;
    size_t m_PopulationSize = 0;
    ACrossover& m_Crossover;
    AMutation& m_Mutation;
};
