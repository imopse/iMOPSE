#include "BNTGPTreeIdentity.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace bntgp
{
    namespace
    {
        constexpr std::uint64_t TreeHashInitialValue =
            0x6a09e667f3bcc909ULL;

        constexpr std::uint64_t IndividualHashInitialValue =
            0x243f6a8885a308d3ULL;

        constexpr std::uint64_t PairTreeMarker =
            0x5041495254524545ULL;

        constexpr std::uint64_t GoldenRatioConstant =
            0x9e3779b97f4a7c15ULL;

        [[nodiscard]]
        std::uint64_t mix64(
            std::uint64_t value) noexcept
        {
            value += GoldenRatioConstant;

            value =
                (value ^ (value >> 30U))
                *
                0xbf58476d1ce4e5b9ULL;

            value =
                (value ^ (value >> 27U))
                *
                0x94d049bb133111ebULL;

            return value ^ (value >> 31U);
        }

        void hashCombine(
            std::uint64_t& seed,
            const std::uint64_t value) noexcept
        {
            seed ^= mix64(
                value
                +
                GoldenRatioConstant
                +
                (seed << 6U)
                +
                (seed >> 2U)
            );
        }

        [[nodiscard]]
        std::uint64_t doubleBits(
            const double value) noexcept
        {
            std::uint64_t bits = 0U;

            static_assert(
                sizeof(bits) == sizeof(value),
                "The cache requires a 64-bit double."
            );

            std::memcpy(
                &bits,
                &value,
                sizeof(bits)
            );

            return bits;
        }

        [[nodiscard]]
        std::uint64_t buildTreeCacheHash(
            const gp::GPTree& tree) noexcept
        {
            std::uint64_t hash = TreeHashInitialValue;

            hashCombine(
                hash,
                static_cast<std::uint64_t>(
                    static_cast<std::int64_t>(tree.rootIndex())
                )
            );

            hashCombine(
                hash,
                static_cast<std::uint64_t>(tree.nodeCount())
            );

            for (const gp::GPNode& node : tree.nodes())
            {
                hashCombine(
                    hash,
                    static_cast<std::uint64_t>(node.kind)
                );

                hashCombine(
                    hash,
                    doubleBits(node.constant)
                );

                hashCombine(
                    hash,
                    static_cast<std::uint64_t>(node.feature)
                );

                hashCombine(
                    hash,
                    static_cast<std::uint64_t>(node.unaryOperation)
                );

                hashCombine(
                    hash,
                    static_cast<std::uint64_t>(node.binaryOperation)
                );

                hashCombine(
                    hash,
                    static_cast<std::uint64_t>(
                        static_cast<std::int64_t>(node.left)
                    )
                );

                hashCombine(
                    hash,
                    static_cast<std::uint64_t>(
                        static_cast<std::int64_t>(node.right)
                    )
                );
            }

            return hash;
        }

        [[nodiscard]]
        bool areNodesIdentical(
            const gp::GPNode& left,
            const gp::GPNode& right) noexcept
        {
            return left.kind == right.kind
                && doubleBits(left.constant) == doubleBits(right.constant)
                && left.feature == right.feature
                && left.unaryOperation == right.unaryOperation
                && left.binaryOperation == right.binaryOperation
                && left.left == right.left
                && left.right == right.right;
        }
    }

    BNTGPEvaluationCacheKey buildBNTGPEvaluationCacheKey(
        const gp::GPTree& tree) noexcept
    {
        std::uint64_t hash = IndividualHashInitialValue;

        hashCombine(hash, PairTreeMarker);
        hashCombine(hash, buildTreeCacheHash(tree));

        return hash;
    }

    bool areBNTGPTreesIdentical(
        const gp::GPTree& left,
        const gp::GPTree& right) noexcept
    {
        if (left.rootIndex() != right.rootIndex()
            || left.nodeCount() != right.nodeCount())
        {
            return false;
        }

        const auto& leftNodes = left.nodes();
        const auto& rightNodes = right.nodes();

        for (std::size_t index = 0U;
            index < leftNodes.size();
            ++index)
        {
            if (!areNodesIdentical(
                leftNodes[index],
                rightNodes[index]))
            {
                return false;
            }
        }

        return true;
    }
}
