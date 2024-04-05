#pragma once

#include "../../problem/AProblem.h"
#include "../../method/AMethod.h"
#include "../../method/configMap/SConfigMap.h"

class CMethodFactory
{
public:
    static AMethod *CreateMethod(
            const char *optimizerConfigPath,
            AProblem &problem
    );
    static void DeleteObjects();
private:
    static SConfigMap *configMap;
    static AInitialization *initialization;
    static ACrossover *crossover;
    static AMutation *mutation;
};
