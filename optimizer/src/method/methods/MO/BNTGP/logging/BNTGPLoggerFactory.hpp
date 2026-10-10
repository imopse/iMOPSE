#pragma once

#include "../config/BNTGPParameters.hpp"
#include "BNTGPLoggerCollection.hpp"

namespace bntgp
{
    [[nodiscard]]
    BNTGPLoggerCollection createBNTGPLoggers(
        const LoggingParameters& parameters
    );
}
