#pragma once

#include "../../../../method/operators/initialization/AInitialization.h"
#include "../../../../method/configMap/SConfigMap.h"

class CInitializationFactory
{
public:
    static AInitialization *Create(SConfigMap *configMap);
};
