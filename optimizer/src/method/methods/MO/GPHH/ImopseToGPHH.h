#pragma once
#include "problem/problems/MSRCPSP/CScheduler.h"

// Twoje struktury:
#include "domain/Instance.hpp"   // u Ciebie mo¿e byæ inna œcie¿ka; jeœli kompilator nie widzi, popraw include.

namespace GPHHAdapter {
    Instance FromScheduler(const CScheduler& sch);
}
