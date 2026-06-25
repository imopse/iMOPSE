#pragma once
#include "method/AMethod.h"
#include "method/configMap/SConfigMap.h"
#include <cstdint>

class CBNTGAGP : public AMethod {
public:
    CBNTGAGP(AProblem* problem, AInitialization* initialization, SConfigMap* cfg);
    void RunOptimization() override;
    void Reset() override {}

private:
    SConfigMap* m_Cfg = nullptr;
    AProblem* m_Problem = nullptr;
    AInitialization* m_Initialization = nullptr;

    bool     m_HasSeedOverride = false;
    uint64_t m_SeedOverride = 0;
};