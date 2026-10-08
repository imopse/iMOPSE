#pragma once
#include "domain/Instance.hpp"

class CScheduler;

namespace gphh_so {

    namespace GPHHAdapter {
        Instance FromScheduler(const ::CScheduler& sch);
    }

}                     