#include "BNTGPVariation.hpp"

#include "../operators/mutation/GPTreeParameterMutation.hpp"

#include <cassert>
#include <random>

namespace bntgp
{
    namespace
    {
        [[nodiscard]]
        bool isValidProbability(
            const double probability) noexcept
        {
            return
                probability >= 0.0
                &&
                probability <= 1.0;
        }

        [[nodiscard]]
        double randomUnit(
            std::mt19937& randomEngine)
        {
            std::uniform_real_distribution<double> distribution(
                0.0,
                1.0
            );

            return distribution(randomEngine);
        }
    }

    BNTGPVariation::BNTGPVariation(
        const VariationParameters& parameters,
        const int maximumDepth) noexcept
        : parameters_(parameters),
        maximumDepth_(maximumDepth)
    {
        assert(
            isValidProbability(
                parameters_.crossoverProbability
            )
        );

        assert(
            isValidProbability(
                parameters_
                .parameterMutationProbability
            )
        );

        assert(
            isValidProbability(
                parameters_
                .structuralMutationProbability
            )
        );


        assert(maximumDepth_ >= 0);
    }

    void BNTGPVariation::apply(
        BNTGPIndividual& firstOffspring,
        BNTGPIndividual& secondOffspring,
        std::mt19937& randomEngine,
        BNTGPVariationTrace* const trace,
        const bool captureTreeSnapshots)
    {
        if (trace != nullptr)
        {
            *trace = BNTGPVariationTrace{};
        }

        if (randomUnit(randomEngine) <
            parameters_.crossoverProbability)
        {
            if (trace != nullptr)
            {
                trace->crossoverSelected = true;
            }

            crossover_.apply(
                firstOffspring,
                secondOffspring,
                maximumDepth_,
                randomEngine
            );

            if (trace != nullptr && captureTreeSnapshots)
            {
                trace->firstChild.postCrossoverTree =
                    firstOffspring.tree();

                trace->secondChild.postCrossoverTree =
                    secondOffspring.tree();
            }
        }

        const auto firstParameterMutationCount =
            gp::mutateTreeParameters(
                firstOffspring,
                parameters_
                    .parameterMutationProbability,
                randomEngine
            );

        if (
            trace != nullptr
            &&
            firstParameterMutationCount > 0U
        )
        {
            trace
                ->firstChild
                .parameterMutationSelected = true;
        }

        const auto secondParameterMutationCount =
            gp::mutateTreeParameters(
                secondOffspring,
                parameters_
                    .parameterMutationProbability,
                randomEngine
            );

        if (
            trace != nullptr
            &&
            secondParameterMutationCount > 0U
        )
        {
            trace
                ->secondChild
                .parameterMutationSelected = true;
        }

        applyStructuralMutation(
            firstOffspring,
            randomEngine,
            trace != nullptr
            ? &trace->firstChild
            : nullptr
        );

        applyStructuralMutation(
            secondOffspring,
            randomEngine,
            trace != nullptr
            ? &trace->secondChild
            : nullptr
        );
    }

    void BNTGPVariation::
        applyStructuralMutation(
            BNTGPIndividual& individual,
            std::mt19937& randomEngine,
            BNTGPChildVariationTrace* const trace)
    {
        const auto mutationResult =
            structuralMutation_.apply(
                individual,
                maximumDepth_,
                parameters_
                    .structuralMutationProbability,
                randomEngine
            );

        if (
            trace != nullptr
            &&
            mutationResult
                .appliedReplacementCount > 0U
        )
        {
            trace->structuralMutation =
                BNTGPStructuralMutationKind::Structural;
        }
    }
}
