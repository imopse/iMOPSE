#pragma once

#include "../../../../../problem/AProblem.h"
#include "../../../../../method/methods/SO/SA/CSA.h"

class CSAFactory
{
public:
    static CSA* CreateSA(SConfigMap* configMap, AProblem& problem, AInitialization* initialization);
    static void DeleteObjects();
private:
    static std::vector<float>* objectiveWeights;
};