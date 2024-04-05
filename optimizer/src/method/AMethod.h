#pragma once

#include "operators/initialization/AInitialization.h"
#include "operators/crossover/ACrossover.h"
#include "operators/mutation/AMutation.h"
#include "../problem/AProblem.h"

class AMethod
{
public:
    static int m_ExperimentRunCounter;

    explicit AMethod(AProblem &evaluator, AInitialization &initialization) : m_Problem(evaluator), m_Initialization(initialization)
    {};
    virtual ~AMethod() = default;

    virtual void RunOptimization() = 0;
    
    virtual void Reset() = 0;

protected:
    AInitialization &m_Initialization;
    AProblem &m_Problem;
};
