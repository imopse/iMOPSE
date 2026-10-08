#pragma once

#include "method/AMethod.h"
#include "method/configMap/SConfigMap.h"

class CBNTGP final : public AMethod
{
public:
    CBNTGP(
        AProblem& problem,
        AInitialization& initialization,
        SConfigMap* configuration
    );

    void RunOptimization() override;

    void Reset() override
    {
    }

private:
    SConfigMap* configuration_{ nullptr };
};