#pragma once

#include "../archive/BNTGPArchive.hpp"
#include "../config/BNTGPParameters.hpp"
#include "../decoding/IBNTGPDecoder.hpp"
#include "../evaluation/BNTGPIndividualEvaluator.hpp"
#include "../logging/BNTGPLoggerCollection.hpp"
#include "../operators/selection/BNTGPGapSelection.hpp"
#include "BNTGPGenerationTrace.hpp"
#include "BNTGPVariation.hpp"

#include <cstdint>
#include <memory>
#include <random>
#include <vector>

namespace bntgp
{
    class BNTGPEvolutionEngine final
    {
    public:
        BNTGPEvolutionEngine(
            std::unique_ptr<IBNTGPDecoder> decoder,
            BNTGPParameters parameters,
            std::uint64_t seed
        );

        void run();

        [[nodiscard]]
        const BNTGPArchive& archive() const noexcept;

        [[nodiscard]]
        const std::vector<BNTGPIndividual>&
            currentPopulation() const noexcept;

        [[nodiscard]]
        std::uint64_t seed() const noexcept;

    private:
        BNTGPParameters parameters_{};
        std::uint64_t seed_{ 0U };

        std::mt19937 randomEngine_;

        BNTGPIndividualEvaluator individualEvaluator_;
        BNTGPGapSelection parentSelection_;
        BNTGPVariation variation_;
        BNTGPLoggerCollection loggers_;

        BNTGPArchive archive_{};

        std::vector<BNTGPIndividual> population_{};
        std::vector<BNTGPIndividual> offspring_{};

        BNTGPIndividualId nextIndividualId_{ 1U };
        BNTGPGenerationTrace generationTrace_{};
    };
}
