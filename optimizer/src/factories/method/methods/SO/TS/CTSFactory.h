#pragma once

#include "../../../../../problem/AProblem.h"
#include "../../../../../method/methods/SO/TS/CTS.h"

class CTSFactory
{
public:
    static CTS* CreateTS(SConfigMap* configMap, AProblem& problem, AInitialization* initialization);
    static void DeleteObjects();
private:
    static std::vector<float>* objectiveWeights;
};