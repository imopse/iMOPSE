#pragma once

#include "../config/BNTGPParameters.hpp"
#include "../core/BNTGPIndividual.hpp"
#include "../operators/crossover/GPTreeCrossover.hpp"
#include "../operators/mutation/GPTreeStructuralMutation.hpp"
#include "BNTGPVariationTrace.hpp"

#include <random>

namespace bntgp
{
    class BNTGPVariation final
    {
    public:
        BNTGPVariation(
            const VariationParameters& parameters,
            int maximumDepth
        ) noexcept;

        void apply(
            BNTGPIndividual& firstOffspring,
            BNTGPIndividual& secondOffspring,
            std::mt19937& randomEngine,
            BNTGPVariationTrace* trace = nullptr,
            bool captureTreeSnapshots = false
        );

    private:
        void applyStructuralMutation(
            BNTGPIndividual& individual,
            std::mt19937& randomEngine,
            BNTGPChildVariationTrace* trace
        );

        VariationParameters parameters_{};
        int maximumDepth_{ 0 };

        gp::GPTreeCrossover crossover_{};
        gp::GPTreeStructuralMutation structuralMutation_{};
    };
}
