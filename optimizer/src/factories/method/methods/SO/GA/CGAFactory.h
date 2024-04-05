#pragma once

#include "../../../../../problem/AProblem.h"
#include "../../../../../method/methods/SO/GA/CGA.h"

class CGAFactory
{
public:
    static CGA *CreateGA(SConfigMap *configMap, AProblem &problem, AInitialization *initialization,
                         ACrossover *crossover,
                         AMutation *mutation);
    static void DeleteObjects();
private:
    static std::vector<float> *objectiveWeights;
    static CFitnessTournament *fitnessTournament;
};
