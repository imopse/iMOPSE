#pragma once

#include "method/configMap/SConfigMap.h"
#include "method/methods/MO/AMOMethod.h"
#include <cstdint>

class CGPHH : public AMOMethod {
public:
    CGPHH(AProblem* problem, AInitialization* initialization, SConfigMap* cfg);
    
    ~CGPHH() {
        delete m_Problem;
        delete m_Initialization;
    };
    
    void RunOptimization() override;
    void Reset() override {}

private:
    SConfigMap* m_Cfg;
    AProblem* m_Problem;
    AInitialization* m_Initialization;

    bool m_HasSeedOverride = false;
    uint64_t m_SeedOverride = 0;
};
