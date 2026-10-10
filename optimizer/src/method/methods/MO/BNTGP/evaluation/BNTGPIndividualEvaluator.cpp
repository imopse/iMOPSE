#include "BNTGPIndividualEvaluator.hpp"

#include <stdexcept>
#include <utility>

namespace bntgp
{
    BNTGPIndividualEvaluator::BNTGPIndividualEvaluator(
        std::unique_ptr<IBNTGPDecoder> decoder,
        const std::size_t initialPopulationSize)
        : decoder_(std::move(decoder))
    {
        if (!decoder_)
        {
            throw std::invalid_argument(
                "BNTGP individual evaluator requires a decoder."
            );
        }

        evaluationCache_.resetForRun(
            initialPopulationSize
        );
    }

    void BNTGPIndividualEvaluator::resetForRun(
        const std::size_t populationSize)
    {
        evaluationCache_.resetForRun(
            populationSize
        );
    }

    std::size_t
        BNTGPIndividualEvaluator::cachedEvaluationCount() const noexcept
    {
        return evaluationCache_.size();
    }

    void BNTGPIndividualEvaluator::evaluate(
        BNTGPIndividual& individual)
    {
        const BNTGPEvaluationCacheKey cacheKey =
            buildBNTGPEvaluationCacheKey(
                individual.tree()
            );

        BNTGPEvaluation cachedEvaluation{};

        if (evaluationCache_.tryGet(
            cacheKey,
            individual.tree(),
            cachedEvaluation))
        {
            individual.setEvaluation(
                cachedEvaluation
            );

            return;
        }

        const BNTGPEvaluation evaluation =
            decoder_->decodeAndEvaluate(
                individual.tree()
            );

        individual.setEvaluation(
            evaluation
        );

        evaluationCache_.store(
            cacheKey,
            individual.tree(),
            evaluation
        );
    }
}
