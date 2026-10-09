#include "BNTGPOffspringGenerator.hpp"

#include "../archive/BNTGPArchive.hpp"
#include "../evaluation/BNTGPIndividualEvaluator.hpp"
#include "../evaluation/BNTGPTreeIdentity.hpp"
#include "../operators/selection/BNTGPGapSelection.hpp"
#include "BNTGPGenerationTrace.hpp"
#include "BNTGPVariation.hpp"

#include <cassert>
#include <cstddef>
#include <limits>
#include <utility>

namespace bntgp
{
    namespace
    {
        [[nodiscard]]
        BNTGPIndividualId takeNextIndividualId(
            BNTGPIndividualId& nextIndividualId) noexcept
        {
            assert(
                nextIndividualId !=
                std::numeric_limits<BNTGPIndividualId>::max()
            );

            const BNTGPIndividualId assignedId =
                nextIndividualId;

            ++nextIndividualId;

            return assignedId;
        }

        void appendLineageRecord(
            BNTGPGenerationTrace* const generationTrace,
            const std::size_t generation,
            const std::size_t matingId,
            const std::size_t childSlot,
            const BNTGPIndividual& child,
            const BNTGPIndividual& firstParent,
            const BNTGPIndividual& secondParent,
            const BNTGPParentPair& parentPair,
            const BNTGPChildVariationTrace& childVariation,
            const bool crossoverSelected,
            const bool captureTreeSnapshots)
        {
            if (generationTrace == nullptr)
            {
                return;
            }

            BNTGPOffspringLineageRecord record{};

            record.generation = generation;
            record.matingId = matingId;
            record.childSlot = childSlot;

            record.childId = child.id();
            record.firstParentId = firstParent.id();
            record.secondParentId = secondParent.id();

            record.firstParentEvaluation =
                firstParent.evaluation();

            record.secondParentEvaluation =
                secondParent.evaluation();

            record.childEvaluation = child.evaluation();

            record.selectedObjective =
                parentPair.objective;

            record.firstParentGap =
                parentPair.firstGap;

            record.secondParentGap =
                parentPair.secondGap;

            record.crossoverSelected =
                crossoverSelected;

            record.parameterMutationSelected =
                childVariation.parameterMutationSelected;

            record.structuralMutation =
                childVariation.structuralMutation;

            record.treeIdentity =
                buildBNTGPEvaluationCacheKey(
                    child.tree()
                );

            record.treeNodeCount =
                child.tree().nodeCount();

            record.treeDepth =
                child.tree().depth();

            if (captureTreeSnapshots)
            {
                record.postCrossoverTree =
                    childVariation.postCrossoverTree;

                record.postParameterMutationTree =
                    childVariation.postParameterMutationTree;

                record.finalTree = child.tree();
            }

            generationTrace->offspring.push_back(
                std::move(record)
            );
        }
    }

    void generateBNTGPOffspring(
        std::vector<BNTGPIndividual>& offspring,
        BNTGPArchive& archive,
        const std::size_t targetPopulationSize,
        BNTGPGapSelection& parentSelection,
        BNTGPVariation& variation,
        BNTGPIndividualEvaluator& evaluator,
        std::mt19937& randomEngine,
        BNTGPIndividualId& nextIndividualId,
        const std::size_t generation,
        BNTGPGenerationTrace* const generationTrace,
        const bool captureTreeSnapshots)
    {
        offspring.clear();

        offspring.reserve(
            targetPopulationSize
        );

        if (generationTrace != nullptr)
        {
            generationTrace->reset(
                generation,
                targetPopulationSize
            );
        }

        const std::vector<BNTGPParentPair>& parentPairs =
            parentSelection.select(
                archive,
                targetPopulationSize,
                randomEngine
            );

        const std::size_t archiveSize =
            archive.size();

        std::size_t matingId = 0U;

        for (const BNTGPParentPair& parentPair :
            parentPairs)
        {
            if (parentPair.firstIndex >= archiveSize ||
                parentPair.secondIndex >= archiveSize)
            {
                ++matingId;
                continue;
            }

            const BNTGPIndividual& firstParent =
                archive
                .entryAt(parentPair.firstIndex)
                .individual();

            const BNTGPIndividual& secondParent =
                archive
                .entryAt(parentPair.secondIndex)
                .individual();

            BNTGPIndividual firstChild =
                firstParent;

            BNTGPIndividual secondChild =
                secondParent;

            firstChild.setId(
                takeNextIndividualId(nextIndividualId)
            );

            secondChild.setId(
                takeNextIndividualId(nextIndividualId)
            );

            BNTGPVariationTrace variationTrace{};

            variation.apply(
                firstChild,
                secondChild,
                randomEngine,
                generationTrace != nullptr
                ? &variationTrace
                : nullptr,
                captureTreeSnapshots
            );

            evaluator.evaluate(
                firstChild
            );

            appendLineageRecord(
                generationTrace,
                generation,
                matingId,
                0U,
                firstChild,
                firstParent,
                secondParent,
                parentPair,
                variationTrace.firstChild,
                variationTrace.crossoverSelected,
                captureTreeSnapshots
            );

            offspring.push_back(
                std::move(firstChild)
            );

            if (offspring.size() <
                targetPopulationSize)
            {
                evaluator.evaluate(
                    secondChild
                );

                appendLineageRecord(
                    generationTrace,
                    generation,
                    matingId,
                    1U,
                    secondChild,
                    firstParent,
                    secondParent,
                    parentPair,
                    variationTrace.secondChild,
                    variationTrace.crossoverSelected,
                    captureTreeSnapshots
                );

                offspring.push_back(
                    std::move(secondChild)
                );
            }

            ++matingId;
        }

        assert(
            offspring.size() <=
            targetPopulationSize
        );

        if (generationTrace != nullptr)
        {
            assert(
                generationTrace->offspring.size() ==
                offspring.size()
            );
        }
    }
}
