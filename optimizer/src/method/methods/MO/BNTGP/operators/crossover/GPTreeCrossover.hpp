#pragma once

#include "../../core/BNTGPIndividual.hpp"
#include "../../gp/GPTreeSubtreeOperations.hpp"

#include <random>

namespace bntgp::gp
{
    class GPTreeCrossover final
    {
    public:
        void apply(
            BNTGPIndividual& firstIndividual,
            BNTGPIndividual& secondIndividual,
            int maximumDepth,
            std::mt19937& randomEngine
        );

    private:
        GPTreeSubtreeOperations subtreeOperations_{};

    };
}