#pragma once

#include "../../../../method/operators/initialization/AInitialization.h"
#include "../../../../method/configMap/SConfigMap.h"
#include "../../../../problem/AProblem.h"

class CInitializationFactory
{
public:
    static AInitialization *Create(SConfigMap *configMap);
    static AInitialization* CreateECVRPTW(SConfigMap* configMap, AProblem& problem);
};
