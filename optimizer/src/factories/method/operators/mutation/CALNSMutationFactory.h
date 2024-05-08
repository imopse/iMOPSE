#pragma once

#include "../../../../method/operators/mutation/AMutation.h"
#include "../../../../method/configMap/SConfigMap.h"
#include "../../../../problem/AProblem.h"
#include <vector>

class CALNSMutationFactory
{
public:
    static std::vector<AMutation*>* CreateRemovalOperators(AProblem& problem, size_t objectiveIndex);
    static std::vector<AMutation*>* CreateInsertionOperators(AProblem& problem, size_t objectiveIndex);
};
