#pragma once

#include "GPTreeSubtreeOperations.hpp"

#include <random>
#include <vector>

namespace bntgp::gp
{
    class GPTreeDepthLimiter final
    {
    public:
        void clamp(
            GPTree& tree,
            int maximumDepth,
            std::mt19937& randomEngine
        );

    private:
        GPTreeSubtreeOperations subtreeOperations_{};

        std::vector<NodeIndex> traversalQueue_{};
        std::vector<int> nodeDepths_{};
    };
}