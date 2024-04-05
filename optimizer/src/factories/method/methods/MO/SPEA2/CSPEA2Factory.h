#pragma once

#include "../../../../../problem/AProblem.h"
#include "../../../../../method/methods/MO/SPEA2/CSPEA2.h"

class CSPEA2Factory
{
public:
    static CSPEA2 *CreateSPEA2(SConfigMap *configMap, AProblem &problem, AInitialization *initialization,
                               ACrossover *crossover,
                               AMutation *mutation);
    static void DeleteObjects();
};
