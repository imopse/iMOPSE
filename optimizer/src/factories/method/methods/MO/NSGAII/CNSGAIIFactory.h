#pragma once

#include "../../../../../problem/AProblem.h"
#include "../../../../../method/methods/MO/NSGAII/CNSGAII.h"
#include "../../../../../method/configMap/SConfigMap.h"

class CNSGAIIFactory
{
public:
    static CNSGAII *CreateNSGAII(SConfigMap *configMap, AProblem &problem, AInitialization *initialization,
                                 ACrossover *crossover,
                                 AMutation *mutation);
    static void DeleteObjects();
private:
    static CRankedTournament *rankedTournament;
};
