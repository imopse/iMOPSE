#pragma once

#include "BNTGPParameters.hpp"

struct SConfigMap;

namespace bntgp
{
    [[nodiscard]]
    BNTGPParameters ReadBNTGPConfiguration(
        const SConfigMap& sourceConfig
    );
}