#include "GPTree.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <utility>
#include <vector>

namespace bntgp::gp
{
    GPTree::GPTree(
        NodeContainer nodes,
        const NodeIndex rootIndex) noexcept
        : nodes_(std::move(nodes)),
        rootIndex_(rootIndex)
    {
    }

    bool GPTree::isEmpty() const noexcept
    {
        return rootIndex_ == InvalidNodeIndex || nodes_.empty();
    }

    bool GPTree::contains(const NodeIndex index) const noexcept
    {
        return index >= 0 &&
            static_cast<std::size_t>(index) < nodes_.size();
    }

    NodeIndex GPTree::rootIndex() const noexcept
    {
        return rootIndex_;
    }

    std::size_t GPTree::nodeCount() const noexcept
    {
        return nodes_.size();
    }

    const GPNode& GPTree::node(const NodeIndex index) const noexcept
    {
        assert(contains(index));

        return nodes_[static_cast<std::size_t>(index)];
    }

    GPNode& GPTree::node(const NodeIndex index) noexcept
    {
        assert(contains(index));

        return nodes_[static_cast<std::size_t>(index)];
    }

    const GPTree::NodeContainer& GPTree::nodes() const noexcept
    {
        return nodes_;
    }

    void GPTree::reserve(const std::size_t capacity)
    {
        nodes_.reserve(capacity);
    }

    NodeIndex GPTree::appendNode(const GPNode& newNode)
    {
        const auto newIndex =
            static_cast<NodeIndex>(nodes_.size());

        nodes_.push_back(newNode);

        return newIndex;
    }

    void GPTree::setRoot(const NodeIndex index) noexcept
    {
        assert(
            index == InvalidNodeIndex ||
            contains(index)
        );

        rootIndex_ = index;
    }

    void GPTree::replaceStorage(
        NodeContainer nodes,
        const NodeIndex rootIndex) noexcept
    {
        nodes_ = std::move(nodes);
        rootIndex_ = rootIndex;
    }

    void GPTree::clear() noexcept
    {
        nodes_.clear();
        rootIndex_ = InvalidNodeIndex;
    }

    bool GPTree::hasAnyFeature() const noexcept
    {
        return std::any_of(
            nodes_.cbegin(),
            nodes_.cend(),
            [](const GPNode& currentNode)
            {
                return currentNode.kind == NodeKind::FEATURE;
            }
        );
    }

    std::size_t GPTree::depthFrom(
        const NodeIndex index) const noexcept
    {
        if (!contains(index))
        {
            return 0U;
        }

        const GPNode& currentNode = node(index);

        switch (currentNode.kind)
        {
        case NodeKind::CONST:
        case NodeKind::FEATURE:
            return 1U;

        case NodeKind::UNARY:
            return 1U + depthFrom(currentNode.left);

        case NodeKind::BINARY:
            return 1U + std::max(
                depthFrom(currentNode.left),
                depthFrom(currentNode.right)
            );
        }

        return 0U;
    }

    std::size_t GPTree::depth() const noexcept
    {
        return isEmpty()
            ? 0U
            : depthFrom(rootIndex_);
    }

    bool GPTree::isStructurallySound() const
    {
        if (isEmpty() || !contains(rootIndex_))
        {
            return false;
        }

        for (const GPNode& currentNode : nodes_)
        {
            switch (currentNode.kind)
            {
            case NodeKind::CONST:
            case NodeKind::FEATURE:
                if (currentNode.left != InvalidNodeIndex ||
                    currentNode.right != InvalidNodeIndex)
                {
                    return false;
                }
                break;

            case NodeKind::UNARY:
                if (!contains(currentNode.left) ||
                    currentNode.right != InvalidNodeIndex)
                {
                    return false;
                }
                break;

            case NodeKind::BINARY:
                if (!contains(currentNode.left) ||
                    !contains(currentNode.right))
                {
                    return false;
                }
                break;

            default:
                return false;
            }
        }

        enum class VisitState : std::uint8_t
        {
            NOT_VISITED,
            ACTIVE,
            FINISHED
        };

        struct TraversalFrame final
        {
            NodeIndex index;
            bool leaving;
        };

        std::vector<VisitState> states(
            nodes_.size(),
            VisitState::NOT_VISITED
        );

        std::vector<TraversalFrame> stack;
        stack.reserve(nodes_.size());
        stack.push_back({ rootIndex_, false });

        while (!stack.empty())
        {
            const TraversalFrame frame = stack.back();
            stack.pop_back();

            const auto position =
                static_cast<std::size_t>(frame.index);

            if (frame.leaving)
            {
                states[position] = VisitState::FINISHED;
                continue;
            }

            if (states[position] == VisitState::ACTIVE)
            {
                return false;
            }

            if (states[position] == VisitState::FINISHED)
            {
                continue;
            }

            states[position] = VisitState::ACTIVE;
            stack.push_back({ frame.index, true });

            const GPNode& currentNode = nodes_[position];

            if (currentNode.kind == NodeKind::UNARY)
            {
                stack.push_back({
                    currentNode.left,
                    false
                    });
            }
            else if (currentNode.kind == NodeKind::BINARY)
            {
                stack.push_back({
                    currentNode.right,
                    false
                    });

                stack.push_back({
                    currentNode.left,
                    false
                    });
            }
        }

        return std::all_of(
            states.cbegin(),
            states.cend(),
            [](const VisitState state)
            {
                return state == VisitState::FINISHED;
            }
        );
    }
}