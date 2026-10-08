#pragma once

#include "method/AMethod.h"
#include "method/configMap/SConfigMap.h"
#include "method/operators/initialization/AInitialization.h"
#include "problem/AProblem.h"

class CBNTGPFactory final
{
public:
    [[nodiscard]]
    static AMethod* CreateBNTGP(
        SConfigMap* configuration,
        AProblem& problem,
        AInitialization* initialization
    );
};