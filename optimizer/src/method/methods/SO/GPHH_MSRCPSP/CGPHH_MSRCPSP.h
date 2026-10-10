#pragma once
#include "method/AMethod.h"
#include "method/configMap/SConfigMap.h"
#include <cstdint>

class CGPHH_MSRCPSP : public AMethod {
public:
    CGPHH_MSRCPSP(AProblem* problem, AInitialization* init, SConfigMap* cfg);
    void RunOptimization() override;
    void Reset() override {}

private:
    AProblem* m_Problem = nullptr;
    SConfigMap* m_Cfg = nullptr;
    bool     m_HasSeedOverride = false;
    uint64_t m_SeedOverride = 0;
};