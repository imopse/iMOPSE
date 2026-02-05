#pragma once
#include <vector>
#include "domain/Instance.hpp"

namespace gp {

    struct CPMPrecalc {
        std::vector<int> est;
        std::vector<int> lst;
        std::vector<int> critLen;
        std::vector<int> totPred;
        std::vector<int> slack;
        std::vector<int> succCount;
        int cmaxCPM = 0;
    };

    bool buildCPM(const Instance& I, CPMPrecalc& out);
    const CPMPrecalc* getCPMPrecalc();
    void setCPMPrecalc(const CPMPrecalc* ptr);

} // namespace gp
