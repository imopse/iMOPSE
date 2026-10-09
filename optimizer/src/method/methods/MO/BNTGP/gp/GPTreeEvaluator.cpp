#include "GPTreeEvaluator.hpp"

#include <cmath>
#include <cstddef>

namespace bntgp::gp
{
    namespace
    {
        constexpr double ProtectedDivisionEpsilon = 1.0e-9;

        [[nodiscard]]
        double protectedDivide(
            const double numerator,
            const double denominator
        ) noexcept
        {
            return std::fabs(denominator) <
                ProtectedDivisionEpsilon
                ? numerator
                : numerator / denominator;
        }

        [[nodiscard]]
        double evaluateNode(
            const NodeIndex nodeIndex,
            const GPTree::NodeContainer& nodes,
            const FeatureValuesView& featureValues
        ) noexcept
        {
            if (nodeIndex < 0 ||
                static_cast<std::size_t>(nodeIndex) >= nodes.size())
            {
                return 0.0;
            }

            const GPNode& currentNode =
                nodes[static_cast<std::size_t>(nodeIndex)];

            switch (currentNode.kind)
            {
            case NodeKind::CONST:
                return currentNode.constant;

            case NodeKind::FEATURE:
                return isValidFeature(currentNode.feature)
                    ? featureValues[currentNode.feature]
                    : 0.0;

            case NodeKind::UNARY:
            {
                const double operand = evaluateNode(
                    currentNode.left,
                    nodes,
                    featureValues
                );

                switch (currentNode.unaryOperation)
                {
                case UnaryOp::NEG:
                    return -operand;

                case UnaryOp::ABS:
                    return std::fabs(operand);
                }

                return operand;
            }

            case NodeKind::BINARY:
            {
                const double leftValue = evaluateNode(
                    currentNode.left,
                    nodes,
                    featureValues
                );

                const double rightValue = evaluateNode(
                    currentNode.right,
                    nodes,
                    featureValues
                );

                switch (currentNode.binaryOperation)
                {
                case BinaryOp::ADD:
                    return leftValue + rightValue;

                case BinaryOp::SUB:
                    return leftValue - rightValue;

                case BinaryOp::MUL:
                    return leftValue * rightValue;

                case BinaryOp::DIV:
                    return protectedDivide(
                        leftValue,
                        rightValue
                    );

                case BinaryOp::MIN:
                    return leftValue < rightValue
                        ? leftValue
                        : rightValue;

                case BinaryOp::MAX:
                    return leftValue > rightValue
                        ? leftValue
                        : rightValue;
                }

                return leftValue;
            }
            }

            return 0.0;
        }
    }

    double evaluateTree(
        const GPTree& tree,
        const FeatureValuesView& featureValues
    ) noexcept
    {
        if (tree.isEmpty())
        {
            return 0.0;
        }

        const GPTree::NodeContainer& nodes = tree.nodes();

        const double result = evaluateNode(
            tree.rootIndex(),
            nodes,
            featureValues
        );

        return std::isfinite(result)
            ? result
            : 0.0;
    }
}