#pragma once
#include "IGPHHConstructive.h"

class CCVRPConstructive : public IGPHHConstructive {
public:
    void BuildSolution(CGPHHIndividual& individual, AProblem& problem) override;
};
