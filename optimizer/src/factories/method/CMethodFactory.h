#pragma once

#include "../../problem/AProblem.h"
#include "../../method/AMethod.h"
#include "../../method/configMap/SConfigMap.h"

class CMethodFactory
{
public:
    static AMethod* CreateMethod(const char* optimizerConfigPath, AProblem* problem);
    static std::vector<float>* ProcessObjectiveWeights(SConfigMap* configMap);
};
