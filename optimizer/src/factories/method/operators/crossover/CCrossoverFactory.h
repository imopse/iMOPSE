#pragma once

#include <set>
#include "../../../../method/operators/crossover/ACrossover.h"
#include "../../../../method/configMap/SConfigMap.h"
#include "../../../../problem/AProblem.h"

class CCrossoverFactory
{
public:
    static ACrossover* Create(SConfigMap* configMap, AProblem* problem);
};
