#pragma once
#include "method/AMethod.h"
#include "method/configMap/SConfigMap.h"

class CGPHH : public AMethod {
public:
    CGPHH(AProblem& problem, AInitialization& init, SConfigMap* cfg);
    void RunOptimization() override;
    void Reset() override {}

private:
    SConfigMap* m_Cfg;
};
