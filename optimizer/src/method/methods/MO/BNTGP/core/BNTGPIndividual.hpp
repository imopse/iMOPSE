#pragma once

#include "../gp/GPTree.hpp"

#include <cstdint>
#include <limits>
#include <utility>

namespace bntgp
{
    using BNTGPIndividualId = std::uint64_t;

    inline constexpr BNTGPIndividualId InvalidBNTGPIndividualId = 0U;

    struct BNTGPEvaluation final
    {
        int makespan{ 0 };
        double cost{ 0.0 };

        double searchMakespan{ 0.0 };
        double searchCost{ 0.0 };
        double rawNormalizedMakespan{ 0.0 };
        double rawNormalizedCost{ 0.0 };
        double normalizedMakespan{ 0.0 };
        double normalizedCost{ 0.0 };

        double fitness{
            std::numeric_limits<double>::infinity()
        };

        void reset() noexcept
        {
            makespan = 0;
            cost = 0.0;

            searchMakespan = 0.0;
            searchCost = 0.0;
            rawNormalizedMakespan = 0.0;
            rawNormalizedCost = 0.0;
            normalizedMakespan = 0.0;
            normalizedCost = 0.0;

            fitness =
                std::numeric_limits<double>::infinity();
        }
    };

    class BNTGPIndividual final
    {
    public:
        BNTGPIndividual() = default;

        explicit BNTGPIndividual(
            gp::GPTree tree
        )
            : tree_(std::move(tree))
        {
        }

        [[nodiscard]]
        BNTGPIndividualId id() const noexcept
        {
            return id_;
        }

        void setId(
            const BNTGPIndividualId id
        ) noexcept
        {
            id_ = id;
        }

        [[nodiscard]]
        const gp::GPTree& tree() const noexcept
        {
            return tree_;
        }

        [[nodiscard]]
        gp::GPTree& treeForModification() noexcept
        {
            evaluation_.reset();
            return tree_;
        }

        [[nodiscard]]
        const BNTGPEvaluation& evaluation() const noexcept
        {
            return evaluation_;
        }

        void setEvaluation(
            const BNTGPEvaluation& evaluation
        ) noexcept
        {
            evaluation_ = evaluation;
        }

        void invalidateEvaluation() noexcept
        {
            evaluation_.reset();
        }

    private:
        BNTGPIndividualId id_{ InvalidBNTGPIndividualId };
        gp::GPTree tree_{};
        BNTGPEvaluation evaluation_{};
    };
}
