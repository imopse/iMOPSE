#include "BNTGPEvolutionEngine.hpp"

#include "BNTGPOffspringGenerator.hpp"
#include "../gp/GPFeatureMask.hpp"
#include "../logging/BNTGPLoggerFactory.hpp"
#include "../operators/initialization/BNTGPPopulationInitializer.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace bntgp
{
    BNTGPEvolutionEngine::
        BNTGPEvolutionEngine(
            std::unique_ptr<IBNTGPDecoder> decoder,
            BNTGPParameters parameters,
            const std::uint64_t seed)
        : parameters_(std::move(parameters)),
        seed_(seed),

        randomEngine_(
            static_cast<unsigned>(seed_)
        ),

        individualEvaluator_(
            std::move(decoder),
            parameters_.evolution.populationSize
        ),

        parentSelection_(
            parameters_.evolution.tournamentSize
        ),

        variation_(
            parameters_.variation,
            parameters_.tree.maximumDepth
        ),

        loggers_(
            createBNTGPLoggers(
                parameters_.logging
            )
        )
    {
        assert(
            parameters_.evolution.populationSize > 0U
        );

        assert(
            parameters_.evolution.tournamentSize > 0
        );

        assert(
            parameters_.tree.maximumDepth >= 0
        );
    }

    void BNTGPEvolutionEngine::run()
    {
        gp::configureGPFeatures(parameters_.enabledFeatures);
        nextIndividualId_ = 1U;

        individualEvaluator_.resetForRun(
            parameters_.evolution.populationSize
        );

        initializeBNTGPPopulation(
            population_,
            parameters_.evolution.populationSize,
            parameters_.tree.maximumDepth,
            randomEngine_,
            individualEvaluator_,
            nextIndividualId_
        );

        archive_.clear();

        archive_.reserve(
            parameters_.evolution.populationSize * 2U
        );

        archive_.updateWithCandidates(
            population_
        );

        if (loggers_.isEnabled(
                BNTGPLogEvent::RunStarted))
        {
            loggers_.onRunStarted(
                BNTGPLoggingContext{
                    0U,
                    parameters_.evolution.generationCount,
                    population_,
                    archive_
                }
            );
        }

        std::size_t completedGenerationCount = 0U;

        const bool captureGenerationTrace =
            loggers_.isEnabled(
                BNTGPLogEvent::GenerationTrace
            );

        const bool captureTreeSnapshots =
            loggers_.requiresTreeSnapshots();

        for (std::size_t generation = 0U;
            generation <
            parameters_.evolution.generationCount;
            ++generation)
        {
            const std::size_t currentGeneration =
                generation + 1U;

            BNTGPGenerationTrace* const trace =
                captureGenerationTrace
                ? &generationTrace_
                : nullptr;

            generateBNTGPOffspring(
                offspring_,
                archive_,
                parameters_.evolution.populationSize,
                parentSelection_,
                variation_,
                individualEvaluator_,
                randomEngine_,
                nextIndividualId_,
                currentGeneration,
                trace,
                captureTreeSnapshots
            );

            if (offspring_.empty())
            {
                break;
            }

            archive_.updateWithCandidates(
                offspring_,
                trace != nullptr
                ? &trace->archiveUpdate
                : nullptr
            );

            if (trace != nullptr)
            {
                assert(
                    trace->offspring.size() ==
                    trace->archiveUpdate
                    .candidateStatuses.size()
                );

                const std::size_t tracedOffspringCount =
                    trace->offspring.size();

                for (std::size_t index = 0U;
                    index < tracedOffspringCount;
                    ++index)
                {
                    trace->offspring[index].archiveStatus =
                        trace->archiveUpdate
                        .candidateStatuses[index];
                }

                loggers_.onGenerationTrace(*trace);
            }

            population_.swap(
                offspring_
            );

            completedGenerationCount = currentGeneration;

            if (loggers_.isEnabled(
                    BNTGPLogEvent::GenerationCompleted))
            {
                loggers_.onGenerationCompleted(
                    BNTGPLoggingContext{
                        currentGeneration,
                        parameters_.evolution.generationCount,
                        population_,
                        archive_
                    }
                );
            }
        }

        if (loggers_.isEnabled(
                BNTGPLogEvent::RunCompleted))
        {
            loggers_.onRunCompleted(
                BNTGPLoggingContext{
                    completedGenerationCount,
                    parameters_.evolution.generationCount,
                    population_,
                    archive_
                }
            );
        }
    }

    const BNTGPArchive&
        BNTGPEvolutionEngine::archive() const noexcept
    {
        return archive_;
    }

    const std::vector<BNTGPIndividual>&
        BNTGPEvolutionEngine::
        currentPopulation() const noexcept
    {
        return population_;
    }

    std::uint64_t
        BNTGPEvolutionEngine::seed() const noexcept
    {
        return seed_;
    }
}
