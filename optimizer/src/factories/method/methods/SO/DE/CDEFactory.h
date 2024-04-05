#pragma once

#include "../../../../../method/methods/SO/DEGR/CDE.h"

class CDEFactory
{
public:
    static CDE* CreateDE(SConfigMap* configMap, AProblem& problem, AInitialization* initialization);
    static void DeleteObjects();
private:
    static std::vector<float>* objectiveWeights;
};
