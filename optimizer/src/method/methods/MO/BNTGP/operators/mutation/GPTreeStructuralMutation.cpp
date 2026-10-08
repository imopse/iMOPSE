#include "GPTreeStructuralMutation.hpp"

#include "../../gp/GPTreeGenerator.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <limits>
#include <random>
#include <vector>

namespace bntgp::gp
{
    namespace
    {
        constexpr std::size_t MaximumAcceptedNodeCount =
            100000U;

        [[nodiscard]]
        NodeIndex copySubtree(
            const GPTree& source,
            const NodeIndex sourceIndex,
            GPTree& destination)
        {
            assert(source.contains(sourceIndex));

            GPNode copiedNode =
                source.node(sourceIndex);

            switch (copiedNode.kind)
            {
            case NodeKind::CONST:
            case NodeKind::FEATURE:
            {
                copiedNode.left =
                    InvalidNodeIndex;

                copiedNode.right =
                    InvalidNodeIndex;

                break;
            }

            case NodeKind::UNARY:
            {
                copiedNode.left =
                    copySubtree(
                        source,
                        copiedNode.left,
                        destination
                    );

                copiedNode.right =
                    InvalidNodeIndex;

                break;
            }

            case NodeKind::BINARY:
            {
                copiedNode.left =
                    copySubtree(
                        source,
                        copiedNode.left,
                        destination
                    );

                copiedNode.right =
                    copySubtree(
                        source,
                        copiedNode.right,
                        destination
                    );

                break;
            }
            }

            return destination.appendNode(
                copiedNode
            );
        }

        [[nodiscard]]
        NodeIndex rebuildWithMutations(
            const GPTree& originalTree,
            const NodeIndex originalIndex,
            const int currentDepth,
            const int maximumDepth,
            const std::vector<unsigned char>& selectedNodes,
            GPTree& mutatedTree,
            std::mt19937& randomEngine,
            std::size_t& appliedReplacementCount)
        {
            assert(
                originalTree.contains(
                    originalIndex
                )
            );

            const auto position =
                static_cast<std::size_t>(
                    originalIndex
                );

            assert(
                position <
                selectedNodes.size()
            );

            if (selectedNodes[position] != 0U)
            {
                                                                                          
                const int availableSubtreeDepth = maximumDepth;

                GPTree donorTree =
                    generateRandomPairTree(
                        randomEngine,
                        availableSubtreeDepth
                    );

                ++appliedReplacementCount;

                return copySubtree(
                    donorTree,
                    donorTree.rootIndex(),
                    mutatedTree
                );
            }

            GPNode copiedNode =
                originalTree.node(
                    originalIndex
                );

            switch (copiedNode.kind)
            {
            case NodeKind::CONST:
            case NodeKind::FEATURE:
            {
                copiedNode.left =
                    InvalidNodeIndex;

                copiedNode.right =
                    InvalidNodeIndex;

                break;
            }

            case NodeKind::UNARY:
            {
                copiedNode.left =
                    rebuildWithMutations(
                        originalTree,
                        copiedNode.left,
                        currentDepth + 1,
                        maximumDepth,
                        selectedNodes,
                        mutatedTree,
                        randomEngine,
                        appliedReplacementCount
                    );

                copiedNode.right =
                    InvalidNodeIndex;

                break;
            }

            case NodeKind::BINARY:
            {
                copiedNode.left =
                    rebuildWithMutations(
                        originalTree,
                        copiedNode.left,
                        currentDepth + 1,
                        maximumDepth,
                        selectedNodes,
                        mutatedTree,
                        randomEngine,
                        appliedReplacementCount
                    );

                copiedNode.right =
                    rebuildWithMutations(
                        originalTree,
                        copiedNode.right,
                        currentDepth + 1,
                        maximumDepth,
                        selectedNodes,
                        mutatedTree,
                        randomEngine,
                        appliedReplacementCount
                    );

                break;
            }
            }

            return mutatedTree.appendNode(
                copiedNode
            );
        }

        [[nodiscard]]
        bool isAcceptableResult(
            const GPTree& tree,
            const int maximumDepth)
        {
            if (
                tree.isEmpty()
                ||
                !tree.isStructurallySound()
                ||
                !tree.hasAnyFeature()
                ||
                tree.nodeCount() >
                    MaximumAcceptedNodeCount || tree.depth() > 256U
            )
            {
                return false;
            }

            (void)maximumDepth;
            return true;
        }
    }

    GPTreeStructuralMutationResult
        GPTreeStructuralMutation::apply(
            BNTGPIndividual& individual,
            const int maximumDepth,
            const double nodeMutationProbability,
            std::mt19937& randomEngine)
    {
        assert(maximumDepth >= 0);

        assert(
            nodeMutationProbability >= 0.0
            &&
            nodeMutationProbability <= 1.0
        );

        GPTreeStructuralMutationResult result{};

        const GPTree& originalTree =
            individual.tree();

        if (
            originalTree.isEmpty()
            ||
            nodeMutationProbability <= 0.0
        )
        {
            return result;
        }

        assert(
            originalTree.isStructurallySound()
        );

        const std::size_t originalNodeCount =
            originalTree.nodeCount();

        std::vector<unsigned char> selectedNodes(
            originalNodeCount,
            0U
        );

        std::bernoulli_distribution shouldMutate(
            nodeMutationProbability
        );

          
                                                                   
          
                             
          
                       
                            
                                             
           
        for (
            std::size_t nodeIndex = 0U;
            nodeIndex < originalNodeCount;
            ++nodeIndex
        )
        {
            if (shouldMutate(randomEngine))
            {
                selectedNodes[nodeIndex] = 1U;

                ++result.selectedNodeCount;
            }
        }

        if (result.selectedNodeCount == 0U)
        {
            return result;
        }

        GPTree mutatedTree;

        mutatedTree.reserve(
            originalNodeCount
        );

        std::size_t appliedReplacementCount =
            0U;

        const NodeIndex mutatedRoot =
            rebuildWithMutations(
                originalTree,
                originalTree.rootIndex(),
                0,
                maximumDepth,
                selectedNodes,
                mutatedTree,
                randomEngine,
                appliedReplacementCount
            );

        mutatedTree.setRoot(
            mutatedRoot
        );

        if (
            !isAcceptableResult(
                mutatedTree,
                maximumDepth
            )
        )
        {
              
                                                       
                                                     
               
            result.appliedReplacementCount = 0U;

            return result;
        }

        GPTree& destinationTree =
            individual.treeForModification();

        destinationTree =
            std::move(mutatedTree);

        result.appliedReplacementCount =
            appliedReplacementCount;

        return result;
    }
}