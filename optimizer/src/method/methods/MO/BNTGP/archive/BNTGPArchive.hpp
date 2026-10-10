#pragma once

#include "BNTGPArchiveEntry.hpp"

#include <cstddef>
#include <vector>

namespace bntgp
{
    struct BNTGPArchiveUpdateTrace;

    class BNTGPArchive final
    {
    public:
        using EntryContainer =
            std::vector<BNTGPArchiveEntry>;

        [[nodiscard]]
        bool empty() const noexcept;

        [[nodiscard]]
        std::size_t size() const noexcept;

        void clear() noexcept;

        void reserve(std::size_t capacity);

        void updateWithCandidates(
            const std::vector<BNTGPIndividual>& candidates,
            BNTGPArchiveUpdateTrace* updateTrace = nullptr
        );

        [[nodiscard]]
        const EntryContainer& entries() const noexcept;

        [[nodiscard]]
        BNTGPArchiveEntry& entryAt(
            std::size_t index
        ) noexcept;

        [[nodiscard]]
        const BNTGPArchiveEntry& entryAt(
            std::size_t index
        ) const noexcept;

    private:
        EntryContainer entries_{};
    };
}
