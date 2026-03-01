#pragma once

#include "../../../../../problem/AProblem.h"
#include "../../../../../method/methods/MO/NSGAII/CNSGAII.h"
#include "../../../../../method/configMap/SConfigMap.h"
#include "../../../../../method/methods/MO/MOEAD/CMOEAD_FRRMAB.h"

class CMOEAD_FRRMABFactory
{
public:
    static CMOEAD_FRRMAB* CreateMOEAD_FRRMAB(SConfigMap* configMap, AProblem& problem, AInitialization* initialization,
                                              ACrossover* crossover, AMutation* mutation);
    static void DeleteObjects();
};
