#pragma once

#include "../archive/BNTGPArchiveUpdateTrace.hpp"
#include "../archive/BNTGPObjectiveComparison.hpp"
#include "../evaluation/BNTGPTreeIdentity.hpp"
#include "BNTGPVariationTrace.hpp"

#include <cstddef>
#include <optional>
#include <vector>

namespace bntgp
{
    struct BNTGPOffspringLineageRecord final
    {
        std::size_t generation{ 0U };
        std::size_t matingId{ 0U };
        std::size_t childSlot{ 0U };

        BNTGPIndividualId childId{
            InvalidBNTGPIndividualId
        };

        BNTGPIndividualId firstParentId{
            InvalidBNTGPIndividualId
        };

        BNTGPIndividualId secondParentId{
            InvalidBNTGPIndividualId
        };

        BNTGPEvaluation firstParentEvaluation{};
        BNTGPEvaluation secondParentEvaluation{};
        BNTGPEvaluation childEvaluation{};

        BNTGPObjective selectedObjective{
            BNTGPObjective::Makespan
        };

        double firstParentGap{ 0.0 };
        double secondParentGap{ 0.0 };

        bool crossoverSelected{ false };
        bool parameterMutationSelected{ false };

        BNTGPStructuralMutationKind structuralMutation{
            BNTGPStructuralMutationKind::None
        };

        BNTGPEvaluationCacheKey treeIdentity{ 0U };
        std::size_t treeNodeCount{ 0U };
        std::size_t treeDepth{ 0U };
        std::optional<gp::GPTree> postCrossoverTree{};
        std::optional<gp::GPTree> postParameterMutationTree{};
        std::optional<gp::GPTree> finalTree{};

        BNTGPArchiveCandidateStatus archiveStatus{
            BNTGPArchiveCandidateStatus::RejectedDominated
        };
    };

    struct BNTGPGenerationTrace final
    {
        std::size_t generation{ 0U };

        std::vector<BNTGPOffspringLineageRecord>
            offspring{};

        BNTGPArchiveUpdateTrace archiveUpdate{};

        void reset(
            const std::size_t currentGeneration,
            const std::size_t expectedOffspringCount)
        {
            generation = currentGeneration;

            offspring.clear();
            offspring.reserve(expectedOffspringCount);

            archiveUpdate.reset(expectedOffspringCount);
        }
    };
}
