#pragma once

#include "../../../../../problem/AProblem.h"
#include "../../../../../method/methods/SO/ACO/TSP-BASED/CACO_TSP.h"

class CACOFactory
{
public:
    static CACO *CreateACO(SConfigMap *configMap, AProblem &problem, AInitialization *initialization,const char *optimizerConfigPath);
    static void DeleteObjects();
private:
    static std::vector<float> *objectiveWeights;
};
