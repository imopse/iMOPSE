#pragma once

#include "../../../../../problem/AProblem.h"
#include "../../../../../method/methods/SO/PSO/CPSO.h"

class CPSOFactory
{
public:
    static CPSO* CreatePSO(SConfigMap* configMap, AProblem& problem, AInitialization* initialization);
    static void DeleteObjects();
private:
    static std::vector<float>* objectiveWeights;
};

