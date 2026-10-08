#pragma once

#include "../../core/BNTGPIndividual.hpp"

#include <cstddef>
#include <random>

namespace bntgp::gp
{
    struct GPTreeStructuralMutationResult final
    {
        std::size_t selectedNodeCount{ 0U };
        std::size_t appliedReplacementCount{ 0U };
    };

    class GPTreeStructuralMutation final
    {
    public:
        [[nodiscard]]
        GPTreeStructuralMutationResult apply(
            BNTGPIndividual& individual,
            int maximumDepth,
            double nodeMutationProbability,
            std::mt19937& randomEngine
        );
    };
}