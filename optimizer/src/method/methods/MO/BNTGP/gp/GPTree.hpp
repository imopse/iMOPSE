#pragma once

#include "GPTypes.hpp"

#include <cstddef>
#include <vector>

namespace bntgp::gp
{
    class GPTree final
    {
    public:
        using NodeContainer = std::vector<GPNode>;

        GPTree() = default;

        GPTree(
            NodeContainer nodes,
            NodeIndex rootIndex
        ) noexcept;

        [[nodiscard]]
        bool isEmpty() const noexcept;

        [[nodiscard]]
        bool contains(NodeIndex index) const noexcept;

        [[nodiscard]]
        NodeIndex rootIndex() const noexcept;

        [[nodiscard]]
        std::size_t nodeCount() const noexcept;

        [[nodiscard]]
        const GPNode& node(NodeIndex index) const noexcept;

        [[nodiscard]]
        GPNode& node(NodeIndex index) noexcept;

        [[nodiscard]]
        const NodeContainer& nodes() const noexcept;

        void reserve(std::size_t capacity);

        [[nodiscard]]
        NodeIndex appendNode(const GPNode& node);

        void setRoot(NodeIndex index) noexcept;

        void replaceStorage(
            NodeContainer nodes,
            NodeIndex rootIndex
        ) noexcept;

        void clear() noexcept;

        [[nodiscard]]
        bool hasAnyFeature() const noexcept;

        [[nodiscard]]
        std::size_t depth() const noexcept;

        [[nodiscard]]
        bool isStructurallySound() const;

    private:
        [[nodiscard]]
        std::size_t depthFrom(NodeIndex index) const noexcept;

        NodeContainer nodes_{};
        NodeIndex rootIndex_{ InvalidNodeIndex };
    };
}