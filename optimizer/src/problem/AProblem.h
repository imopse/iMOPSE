#pragma once

#include "SProblemEncoding.h"
#include "../method/individual/SGenotype.h"
#include "../method/individual/AIndividual.h"

class AProblem
{
public:
    virtual SProblemEncoding &GetProblemEncoding() = 0;
    virtual void Evaluate(AIndividual& individual) = 0;
    virtual void LogSolution(AIndividual& individual) = 0;
    virtual void LogAdditionalData() = 0;
};
