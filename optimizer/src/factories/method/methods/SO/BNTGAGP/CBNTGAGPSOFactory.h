#pragma once
#include "method/AMethod.h"
#include "method/configMap/SConfigMap.h"
#include "problem/AProblem.h"
#include "method/operators/initialization/AInitialization.h"

class CBNTGAGPSOFactory {
public:
    static AMethod* CreateBNTGAGPSO(SConfigMap* cfg, AProblem& problem, AInitialization* init);
};