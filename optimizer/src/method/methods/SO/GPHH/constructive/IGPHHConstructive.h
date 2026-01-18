#pragma once

#include "../../../../../problem/AProblem.h"

class CGPHHIndividual;

class IGPHHConstructive {
public:
    virtual ~IGPHHConstructive() = default;
    virtual void BuildSolution(CGPHHIndividual& individual, AProblem& problem) = 0;
};
