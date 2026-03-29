#pragma once
#include "domain/Instance.hpp"

class CScheduler;

namespace gpbntga_so {

    namespace BNTGAGPAdapter {
        Instance FromScheduler(const ::CScheduler& sch);
    }

} // namespace gpbntga_so