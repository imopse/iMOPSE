#pragma once

#include "../../core/BNTGPIndividual.hpp"

#include <cstddef>
#include <random>

namespace bntgp::gp
{
    [[nodiscard]]
    std::size_t mutateTreeParameters(
        BNTGPIndividual& individual,
        double nodeMutationProbability,
        std::mt19937& randomEngine
    );
}