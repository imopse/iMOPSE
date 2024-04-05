#pragma once

#include "../../../../../problem/AProblem.h"
#include "../../../../../method/methods/MO/NSGAII/CNSGAII.h"
#include "../../../../../method/configMap/SConfigMap.h"
#include "../../../../../method/methods/MO/MOEAD/CMOEAD.h"

class CMOEADFactory
{
public:
    static CMOEAD *CreateMOEAD(SConfigMap *configMap, AProblem &problem, AInitialization *initialization,
                                 ACrossover *crossover,
                                 AMutation *mutation);
    static void DeleteObjects();
};
