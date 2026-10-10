#pragma once

#include "../gp/GPTree.hpp"

#include <cstdint>

namespace bntgp
{
    using BNTGPEvaluationCacheKey = std::uint64_t;

    [[nodiscard]]
    BNTGPEvaluationCacheKey buildBNTGPEvaluationCacheKey(
        const gp::GPTree& tree
    ) noexcept;

    [[nodiscard]]
    bool areBNTGPTreesIdentical(
        const gp::GPTree& left,
        const gp::GPTree& right
    ) noexcept;
}
