#include "GPTreeDepthLimiter.hpp"

#include "FeatureCatalog.hpp"

#include <cassert>
#include <cstddef>
#include <limits>
#include <random>

namespace bntgp::gp
{
    namespace
    {
        constexpr double FeatureLeafProbability = 0.90;

        constexpr double MinimumConstantValue = -1.0;
        constexpr double MaximumConstantValue = 1.0;

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

        [[nodiscard]]
        int randomInt(
            std::mt19937& randomEngine,
            const int minimum,
            const int maximum)
        {
            std::uniform_int_distribution<int> distribution(
                minimum,
                maximum
            );

            return distribution(randomEngine);
        }

        [[nodiscard]]
        FeatureId sampleFeature(
            std::mt19937& randomEngine)
        {
            static_assert(
                FeatureSamplingOrder.size() <=
                static_cast<std::size_t>(
                    std::numeric_limits<int>::max()
                    )
                );

            const int featureIndex = randomInt(
                randomEngine,
                0,
                static_cast<int>(
                    FeatureSamplingOrder.size()
                    ) - 1
            );

            return FeatureSamplingOrder[
                static_cast<std::size_t>(featureIndex)
            ];
        }

        [[nodiscard]]
        GPNode makeRandomLeaf(
            std::mt19937& randomEngine)
        {
            GPNode leaf{};

            if (randomUnit(randomEngine) <
                FeatureLeafProbability)
            {
                leaf.kind = NodeKind::FEATURE;
                leaf.feature =
                    sampleFeature(randomEngine);
            }
            else
            {
                leaf.kind = NodeKind::CONST;

                std::uniform_real_distribution<double> distribution(
                    MinimumConstantValue,
                    MaximumConstantValue
                );

                leaf.constant =
                    distribution(randomEngine);
            }

            leaf.left = InvalidNodeIndex;
            leaf.right = InvalidNodeIndex;

            return leaf;
        }

        void replaceWithSingleNode(
            GPTree& tree,
            const GPNode& node)
        {
            tree.clear();

            const NodeIndex rootIndex =
                tree.appendNode(node);

            tree.setRoot(rootIndex);
        }
    }

    void GPTreeDepthLimiter::clamp(
        GPTree& tree,
        const int maximumDepth,
        std::mt19937& randomEngine)
    {
        assert(maximumDepth >= 0);

        if (tree.isEmpty())
        {
            return;
        }

        const int allowedDepth =
            maximumDepth + 1;

        const std::size_t currentTreeDepth =
            tree.depth();

        if (currentTreeDepth <=
            static_cast<std::size_t>(allowedDepth))
        {
            return;
        }

        traversalQueue_.clear();
        traversalQueue_.reserve(
            tree.nodeCount()
        );

        nodeDepths_.assign(
            tree.nodeCount(),
            -1
        );

        const NodeIndex rootIndex =
            tree.rootIndex();

        assert(tree.contains(rootIndex));

        traversalQueue_.push_back(
            rootIndex
        );

        nodeDepths_[
            static_cast<std::size_t>(rootIndex)
        ] = 0;

        for (std::size_t queuePosition = 0U;
            queuePosition < traversalQueue_.size();
            ++queuePosition)
        {
            const NodeIndex currentIndex =
                traversalQueue_[queuePosition];

            const int currentDepth =
                nodeDepths_[
                    static_cast<std::size_t>(
                        currentIndex
                        )
                ];

            GPNode& currentNode =
                tree.node(currentIndex);

            if (currentDepth >= allowedDepth - 1)
            {
                if (currentNode.kind == NodeKind::UNARY ||
                    currentNode.kind == NodeKind::BINARY)
                {
                    currentNode =
                        makeRandomLeaf(randomEngine);
                }

                continue;
            }

            const auto enqueueChild =
                [&](const NodeIndex childIndex)
                {
                    if (!tree.contains(childIndex))
                    {
                        return;
                    }

                    const auto childPosition =
                        static_cast<std::size_t>(
                            childIndex
                            );

                    if (nodeDepths_[childPosition] != -1)
                    {
                        return;
                    }

                    nodeDepths_[childPosition] =
                        currentDepth + 1;

                    traversalQueue_.push_back(
                        childIndex
                    );
                };

            if (currentNode.kind == NodeKind::UNARY)
            {
                enqueueChild(
                    currentNode.left
                );
            }
            else if (
                currentNode.kind == NodeKind::BINARY)
            {
                enqueueChild(
                    currentNode.left
                );

                enqueueChild(
                    currentNode.right
                );
            }
        }

        tree = subtreeOperations_.extractSubtree(
            tree,
            tree.rootIndex()
        );

        if (!tree.hasAnyFeature())
        {
            GPNode featureLeaf{};
            featureLeaf.kind = NodeKind::FEATURE;
            featureLeaf.feature =
                sampleFeature(randomEngine);

            featureLeaf.left =
                InvalidNodeIndex;

            featureLeaf.right =
                InvalidNodeIndex;

            replaceWithSingleNode(
                tree,
                featureLeaf
            );
        }

        if (tree.depth() >
            static_cast<std::size_t>(allowedDepth))
        {
            replaceWithSingleNode(
                tree,
                makeRandomLeaf(randomEngine)
            );
        }
    }
}