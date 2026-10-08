#pragma once

#include "../core/BNTGPIndividual.hpp"

#include <cstddef>
#include <utility>

namespace bntgp
{
    class BNTGPArchiveEntry final
    {
    public:
        explicit BNTGPArchiveEntry(
            BNTGPIndividual individual
        )
            : individual_(std::move(individual))
        {
        }

        [[nodiscard]]
        const BNTGPIndividual& individual() const noexcept
        {
            return individual_;
        }

        [[nodiscard]]
        std::size_t selectionCount() const noexcept
        {
            return selectionCount_;
        }

        void recordSelection() noexcept
        {
            ++selectionCount_;
        }

        void resetSelectionCount() noexcept
        {
            selectionCount_ = 0U;
        }

    private:
        BNTGPIndividual individual_;
        std::size_t selectionCount_{ 0U };
    };
}