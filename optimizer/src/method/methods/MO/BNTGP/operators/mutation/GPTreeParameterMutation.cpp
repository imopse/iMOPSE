#include "GPTreeParameterMutation.hpp"

#include "../../gp/FeatureCatalog.hpp"
#include "../../gp/GPFeatureMask.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <limits>
#include <random>

namespace bntgp::gp
{
    namespace
    {
        inline constexpr double ConstantLowerBound = -1.0;
        inline constexpr double ConstantUpperBound = 1.0;
        inline constexpr double ConstantMutationSigma = 1.0;

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
        FeatureId sampleFeature(std::mt19937& randomEngine)
        {
            return sampleActiveGPFeature(randomEngine);
        }

        [[nodiscard]]
        BinaryOp sampleBinaryOperation(std::mt19937& randomEngine)
        {
            std::uniform_int_distribution<int> distribution(
                0, static_cast<int>(BinaryOperationSamplingOrder.size()) - 1);
            return BinaryOperationSamplingOrder[static_cast<std::size_t>(distribution(randomEngine))];
        }

        [[nodiscard]]
        double sampleBoundedGaussianConstant(
            const double currentConstant,
            std::mt19937& randomEngine)
        {
            double center = currentConstant;

            if (center < ConstantLowerBound)
            {
                center = ConstantLowerBound;
            }
            else if (center > ConstantUpperBound)
            {
                center = ConstantUpperBound;
            }

            std::normal_distribution<double> gaussianNoise(
                0.0,
                ConstantMutationSigma
            );

            for (;;)
            {
                const double candidate =
                    center + gaussianNoise(randomEngine);

                if (
                    candidate >= ConstantLowerBound
                    &&
                    candidate <= ConstantUpperBound
                )
                {
                    return candidate;
                }
            }
        }

        void mutateNodeParameter(
            GPNode& selectedNode,
            std::mt19937& randomEngine)
        {
            switch (selectedNode.kind)
            {
            case NodeKind::CONST:
            {
                selectedNode.constant =
                    sampleBoundedGaussianConstant(
                        selectedNode.constant,
                        randomEngine
                    );

                break;
            }

            case NodeKind::FEATURE:
            {
                selectedNode.feature =
                    sampleFeature(randomEngine);

                break;
            }

            case NodeKind::UNARY:
            {
                selectedNode.kind =
                    NodeKind::FEATURE;

                selectedNode.left =
                    InvalidNodeIndex;

                selectedNode.right =
                    InvalidNodeIndex;

                selectedNode.feature =
                    sampleFeature(randomEngine);

                break;
            }

            case NodeKind::BINARY:
            {
                selectedNode.binaryOperation =
                    sampleBinaryOperation(randomEngine);

                break;
            }
            }
        }
    }

    std::size_t mutateTreeParameters(
        BNTGPIndividual& individual,
        const double nodeMutationProbability,
        std::mt19937& randomEngine)
    {
        assert(
            nodeMutationProbability >= 0.0
            &&
            nodeMutationProbability <= 1.0
        );

        const std::size_t nodeCount =
            individual.tree().nodeCount();

        if (
            nodeCount == 0U
            ||
            nodeMutationProbability <= 0.0
        )
        {
            return 0U;
        }

        assert(
            nodeCount <=
            static_cast<std::size_t>(
                std::numeric_limits<NodeIndex>::max()
            )
        );

        std::bernoulli_distribution shouldMutate(
            nodeMutationProbability
        );

        GPTree* mutableTree = nullptr;
        std::size_t selectedNodeCount = 0U;

        for (
            std::size_t rawIndex = 0U;
            rawIndex < nodeCount;
            ++rawIndex
        )
        {
            if (!shouldMutate(randomEngine))
            {
                continue;
            }

            if (mutableTree == nullptr)
            {
                mutableTree =
                    &individual.treeForModification();
            }

            GPNode& selectedNode =
                mutableTree->node(
                    static_cast<NodeIndex>(rawIndex)
                );

            mutateNodeParameter(
                selectedNode,
                randomEngine
            );

            ++selectedNodeCount;
        }

        return selectedNodeCount;
    }
}