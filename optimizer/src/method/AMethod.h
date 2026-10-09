#pragma once

#include "operators/initialization/AInitialization.h"
#include "operators/crossover/ACrossover.h"
#include "operators/mutation/AMutation.h"
#include "../problem/AProblem.h"

class AMethod
{
public:
    virtual ~AMethod() = default;
    static int m_ExperimentRunCounter;
    
    virtual void RunOptimization() = 0;
    
    virtual void Reset() = 0;
};
