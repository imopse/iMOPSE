#include "GPTreeCrossover.hpp"

#include <cassert>
#include <cstddef>
#include <limits>
#include <random>
#include <utility>

namespace bntgp::gp
{
    namespace
    {
        constexpr std::size_t MaximumAcceptedNodeCount =
            100000U;

        [[nodiscard]]
        NodeIndex pickRandomNode(
            const GPTree& tree,
            std::mt19937& randomEngine)
        {
            if (tree.isEmpty())
            {
                return InvalidNodeIndex;
            }

            assert(
                tree.nodeCount() <=
                static_cast<std::size_t>(
                    std::numeric_limits<int>::max()
                    )
            );

            std::uniform_int_distribution<int> distribution(
                0,
                static_cast<int>(tree.nodeCount()) - 1
            );

            return static_cast<NodeIndex>(
                distribution(randomEngine)
                );
        }

        [[nodiscard]]
        bool isAcceptableChild(
            const GPTree& tree)
        {
            return
                !tree.isEmpty()
                &&
                tree.nodeCount() <=
                MaximumAcceptedNodeCount
                &&
                tree.isStructurallySound();
        }
    }

    void GPTreeCrossover::apply(
        BNTGPIndividual& firstIndividual,
        BNTGPIndividual& secondIndividual,
        const int maximumDepth,
        std::mt19937& randomEngine)
    {
        assert(maximumDepth >= 0);
        (void)maximumDepth;

        if (firstIndividual.tree().isEmpty() ||
            secondIndividual.tree().isEmpty())
        {
            return;
        }

        GPTree& firstTree =
            firstIndividual.treeForModification();

        GPTree& secondTree =
            secondIndividual.treeForModification();

        GPTree firstOriginal =
            std::move(firstTree);

        GPTree secondOriginal =
            std::move(secondTree);

        const NodeIndex firstSelectedNode =
            pickRandomNode(
                firstOriginal,
                randomEngine
            );

        const NodeIndex secondSelectedNode =
            pickRandomNode(
                secondOriginal,
                randomEngine
            );

        if (firstSelectedNode == InvalidNodeIndex ||
            secondSelectedNode == InvalidNodeIndex)
        {
            firstTree =
                std::move(firstOriginal);

            secondTree =
                std::move(secondOriginal);

            return;
        }

        GPTree firstSubtree =
            subtreeOperations_.extractSubtree(
                firstOriginal,
                firstSelectedNode
            );

        GPTree secondSubtree =
            subtreeOperations_.extractSubtree(
                secondOriginal,
                secondSelectedNode
            );



        GPTree firstChild =
            subtreeOperations_.graftSubtree(
                firstOriginal,
                firstSelectedNode,
                secondSubtree
            );

        GPTree secondChild =
            subtreeOperations_.graftSubtree(
                secondOriginal,
                secondSelectedNode,
                firstSubtree
            );



        if (isAcceptableChild(firstChild) &&
            isAcceptableChild(secondChild))
        {
            firstTree =
                std::move(firstChild);

            secondTree =
                std::move(secondChild);

            return;
        }

        firstTree =
            std::move(firstOriginal);

        secondTree =
            std::move(secondOriginal);
    }
}