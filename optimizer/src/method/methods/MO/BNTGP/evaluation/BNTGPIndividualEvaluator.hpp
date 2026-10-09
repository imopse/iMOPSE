#pragma once

#include "../core/BNTGPIndividual.hpp"
#include "../decoding/IBNTGPDecoder.hpp"
#include "BNTGPEvaluationCache.hpp"

#include <cstddef>
#include <memory>

namespace bntgp
{
    class BNTGPIndividualEvaluator final
    {
    public:
        BNTGPIndividualEvaluator(
            std::unique_ptr<IBNTGPDecoder> decoder,
            std::size_t initialPopulationSize
        );

        void resetForRun(
            std::size_t populationSize
        );

        void evaluate(
            BNTGPIndividual& individual
        );

        [[nodiscard]]
        std::size_t cachedEvaluationCount() const noexcept;

    private:
        std::unique_ptr<IBNTGPDecoder> decoder_;
        BNTGPEvaluationCache evaluationCache_{};
    };
}
