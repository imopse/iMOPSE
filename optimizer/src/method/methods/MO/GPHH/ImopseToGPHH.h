#pragma once
#include "problem/problems/MSRCPSP/CScheduler.h"
#include "domain/Instance.hpp"

namespace GPHHAdapter {
    Instance FromScheduler(const CScheduler& sch);
}
