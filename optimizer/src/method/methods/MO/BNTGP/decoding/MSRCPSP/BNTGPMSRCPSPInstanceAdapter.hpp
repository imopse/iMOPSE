#pragma once

#include "domain/Instance.hpp"

class CScheduler;

namespace bntgp::decoding::msrcpsp
{
    [[nodiscard]]
    domain::Instance fromScheduler(
        const ::CScheduler& scheduler
    );
}
