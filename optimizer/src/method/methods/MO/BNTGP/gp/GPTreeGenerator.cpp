#include "GPTreeGenerator.hpp"

#include "FeatureCatalog.hpp"
#include "GPFeatureMask.hpp"

#include <algorithm>
#include <cassert>
#include <array>
#include <cstddef>
#include <random>

namespace bntgp::gp
{
    namespace
    {
        constexpr double LeafProbability = 0.25;
        constexpr double FeatureTerminalProbability = 0.70;

        constexpr double MinimumConstantValue = -1.0;
        constexpr double MaximumConstantValue = 1.0;

        constexpr std::size_t MaximumInitialReserve = 127U;

        inline constexpr std::array<BinaryOp, 6U>
            BinaryOperationSamplingOrder{
                BinaryOp::ADD,
                BinaryOp::SUB,
                BinaryOp::MUL,
                BinaryOp::DIV,
                BinaryOp::MIN,
                BinaryOp::MAX
        };

        [[nodiscard]]
        std::size_t calculateReserveHint(
            const int maximumDepth
        ) noexcept
        {
            std::size_t totalNodeCount = 1U;
            std::size_t nodesOnLevel = 1U;

            for (int level = 0;
                level < maximumDepth &&
                totalNodeCount < MaximumInitialReserve;
                ++level)
            {
                nodesOnLevel *= 2U;

                totalNodeCount = std::min(
                    MaximumInitialReserve,
                    totalNodeCount + nodesOnLevel
                );
            }

            return totalNodeCount;
        }

        [[nodiscard]]
        BinaryOp sampleBinaryOperation(
            std::mt19937& randomEngine)
        {
            std::uniform_int_distribution<int> distribution(
                0,
                static_cast<int>(
                    BinaryOperationSamplingOrder.size()
                    ) - 1
            );

            return BinaryOperationSamplingOrder[
                static_cast<std::size_t>(
                    distribution(randomEngine)
                    )
            ];
        }

        [[nodiscard]]
        FeatureId sampleFeature(std::mt19937& randomEngine)
        {
            return sampleActiveGPFeature(randomEngine);
        }

        [[nodiscard]]
        NodeIndex growPairTree(
            std::mt19937& randomEngine,
            GPTree& tree,
            const int currentDepth,
            const int maximumDepth)
        {
            std::uniform_real_distribution<double>
                unitDistribution(0.0, 1.0);

            if (currentDepth == maximumDepth ||
                unitDistribution(randomEngine) < LeafProbability)
            {
                GPNode terminalNode{};

                if (unitDistribution(randomEngine) <
                    FeatureTerminalProbability)
                {
                    terminalNode.kind = NodeKind::FEATURE;
                    terminalNode.feature =
                        sampleFeature(randomEngine);
                }
                else
                {
                    terminalNode.kind = NodeKind::CONST;

                    const double unitValue =
                        unitDistribution(randomEngine);

                    terminalNode.constant =
                        MinimumConstantValue +
                        unitValue *
                        (
                            MaximumConstantValue -
                            MinimumConstantValue
                            );
                }

                return tree.appendNode(terminalNode);
            }

            GPNode operationNode{};
            operationNode.kind = NodeKind::BINARY;
            operationNode.binaryOperation =
                sampleBinaryOperation(randomEngine);

            operationNode.left = growPairTree(
                randomEngine,
                tree,
                currentDepth + 1,
                maximumDepth
            );

            operationNode.right = growPairTree(
                randomEngine,
                tree,
                currentDepth + 1,
                maximumDepth
            );

            return tree.appendNode(operationNode);
        }
    }

    GPTree generateRandomPairTree(
        std::mt19937& randomEngine,
        const int maximumDepth)
    {
        assert(maximumDepth >= 0);

        GPTree tree;
        tree.reserve(
            calculateReserveHint(maximumDepth)
        );

        do
        {
            tree.clear();

            const NodeIndex rootIndex = growPairTree(
                randomEngine,
                tree,
                0,
                maximumDepth
            );

            tree.setRoot(rootIndex);
        } while (!tree.hasAnyFeature());

        return tree;
    }
}