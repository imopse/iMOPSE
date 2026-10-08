#pragma once

#include "GPTree.hpp"

#include <vector>

namespace bntgp::gp
{
    class GPTreeSubtreeOperations final
    {
    public:
        void collectNodeIndices(
            const GPTree& tree,
            NodeIndex subtreeRoot,
            std::vector<NodeIndex>& output
        ) const;

        [[nodiscard]]
        GPTree extractSubtree(
            const GPTree& source,
            NodeIndex subtreeRoot
        );

        [[nodiscard]]
        GPTree graftSubtree(
            const GPTree& host,
            NodeIndex replacedNode,
            const GPTree& donor
        );

    private:
        std::vector<NodeIndex> traversalOrder_{};

        std::vector<NodeIndex> hostRemap_{};

        std::vector<NodeIndex> donorRemap_{};
    };
}