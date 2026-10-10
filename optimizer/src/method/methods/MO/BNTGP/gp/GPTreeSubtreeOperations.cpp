#include "GPTreeSubtreeOperations.hpp"

#include <cassert>
#include <cstddef>
#include <limits>
#include <utility>

namespace bntgp::gp
{
    namespace
    {
        [[nodiscard]]
        NodeIndex appendNode(
            GPTree::NodeContainer& output,
            const GPNode& node)
        {
            assert(
                output.size() <=
                static_cast<std::size_t>(
                    std::numeric_limits<NodeIndex>::max()
                    )
            );

            const NodeIndex newIndex =
                static_cast<NodeIndex>(output.size());

            output.push_back(node);

            return newIndex;
        }

        void collectPreOrder(
            const GPTree& tree,
            const NodeIndex nodeIndex,
            std::vector<NodeIndex>& output)
        {
            if (!tree.contains(nodeIndex))
            {
                return;
            }

            output.push_back(nodeIndex);

            const GPNode& node = tree.node(nodeIndex);

            switch (node.kind)
            {
            case NodeKind::CONST:
            case NodeKind::FEATURE:
                return;

            case NodeKind::UNARY:
                collectPreOrder(
                    tree,
                    node.left,
                    output
                );
                return;

            case NodeKind::BINARY:
                collectPreOrder(
                    tree,
                    node.left,
                    output
                );

                collectPreOrder(
                    tree,
                    node.right,
                    output
                );
                return;
            }
        }

        [[nodiscard]]
        NodeIndex clonePostOrder(
            const GPTree& source,
            const NodeIndex sourceIndex,
            GPTree::NodeContainer& output,
            std::vector<NodeIndex>& remap)
        {
            if (!source.contains(sourceIndex))
            {
                return InvalidNodeIndex;
            }

            const auto sourcePosition =
                static_cast<std::size_t>(sourceIndex);

            if (remap[sourcePosition] != InvalidNodeIndex)
            {
                return remap[sourcePosition];
            }

            const GPNode& sourceNode =
                source.node(sourceIndex);

            GPNode copiedNode = sourceNode;

            switch (sourceNode.kind)
            {
            case NodeKind::CONST:
            case NodeKind::FEATURE:
                copiedNode.left = InvalidNodeIndex;
                copiedNode.right = InvalidNodeIndex;
                break;

            case NodeKind::UNARY:
                copiedNode.left = clonePostOrder(
                    source,
                    sourceNode.left,
                    output,
                    remap
                );

                copiedNode.right = InvalidNodeIndex;
                break;

            case NodeKind::BINARY:
                copiedNode.left = clonePostOrder(
                    source,
                    sourceNode.left,
                    output,
                    remap
                );

                copiedNode.right = clonePostOrder(
                    source,
                    sourceNode.right,
                    output,
                    remap
                );
                break;
            }

            const NodeIndex copiedIndex =
                appendNode(output, copiedNode);

            remap[sourcePosition] = copiedIndex;

            return copiedIndex;
        }

        [[nodiscard]]
        NodeIndex cloneHostWithReplacement(
            const GPTree& host,
            const NodeIndex hostIndex,
            const NodeIndex replacedNode,
            const GPTree& donor,
            GPTree::NodeContainer& output,
            std::vector<NodeIndex>& hostRemap,
            std::vector<NodeIndex>& donorRemap)
        {
            if (!host.contains(hostIndex))
            {
                return InvalidNodeIndex;
            }

            if (hostIndex == replacedNode)
            {
                return clonePostOrder(
                    donor,
                    donor.rootIndex(),
                    output,
                    donorRemap
                );
            }

            const auto hostPosition =
                static_cast<std::size_t>(hostIndex);

            if (hostRemap[hostPosition] !=
                InvalidNodeIndex)
            {
                return hostRemap[hostPosition];
            }

            const GPNode& hostNode =
                host.node(hostIndex);

            GPNode copiedNode = hostNode;

            switch (hostNode.kind)
            {
            case NodeKind::CONST:
            case NodeKind::FEATURE:
                copiedNode.left = InvalidNodeIndex;
                copiedNode.right = InvalidNodeIndex;
                break;

            case NodeKind::UNARY:
                copiedNode.left =
                    cloneHostWithReplacement(
                        host,
                        hostNode.left,
                        replacedNode,
                        donor,
                        output,
                        hostRemap,
                        donorRemap
                    );

                copiedNode.right = InvalidNodeIndex;
                break;

            case NodeKind::BINARY:
                copiedNode.left =
                    cloneHostWithReplacement(
                        host,
                        hostNode.left,
                        replacedNode,
                        donor,
                        output,
                        hostRemap,
                        donorRemap
                    );

                copiedNode.right =
                    cloneHostWithReplacement(
                        host,
                        hostNode.right,
                        replacedNode,
                        donor,
                        output,
                        hostRemap,
                        donorRemap
                    );
                break;
            }

            const NodeIndex copiedIndex =
                appendNode(output, copiedNode);

            hostRemap[hostPosition] = copiedIndex;

            return copiedIndex;
        }
    }

    void GPTreeSubtreeOperations::collectNodeIndices(
        const GPTree& tree,
        const NodeIndex subtreeRoot,
        std::vector<NodeIndex>& output) const
    {
        output.clear();

        if (!tree.contains(subtreeRoot))
        {
            return;
        }

        output.reserve(tree.nodeCount());

        collectPreOrder(
            tree,
            subtreeRoot,
            output
        );
    }

    GPTree GPTreeSubtreeOperations::extractSubtree(
        const GPTree& source,
        const NodeIndex subtreeRoot)
    {
        if (!source.contains(subtreeRoot))
        {
            return {};
        }

        traversalOrder_.clear();
        traversalOrder_.reserve(source.nodeCount());

        collectPreOrder(
            source,
            subtreeRoot,
            traversalOrder_
        );

        hostRemap_.assign(
            source.nodeCount(),
            InvalidNodeIndex
        );

        for (std::size_t newPosition = 0U;
            newPosition < traversalOrder_.size();
            ++newPosition)
        {
            assert(
                newPosition <=
                static_cast<std::size_t>(
                    std::numeric_limits<NodeIndex>::max()
                    )
            );

            const NodeIndex oldIndex =
                traversalOrder_[newPosition];

            hostRemap_[
                static_cast<std::size_t>(oldIndex)
            ] = static_cast<NodeIndex>(newPosition);
        }

        GPTree::NodeContainer extractedNodes;
        extractedNodes.resize(
            traversalOrder_.size()
        );

        for (std::size_t newPosition = 0U;
            newPosition < traversalOrder_.size();
            ++newPosition)
        {
            const NodeIndex oldIndex =
                traversalOrder_[newPosition];

            const GPNode& oldNode =
                source.node(oldIndex);

            GPNode copiedNode = oldNode;

            switch (oldNode.kind)
            {
            case NodeKind::CONST:
            case NodeKind::FEATURE:
                copiedNode.left = InvalidNodeIndex;
                copiedNode.right = InvalidNodeIndex;
                break;

            case NodeKind::UNARY:
                copiedNode.left =
                    oldNode.left == InvalidNodeIndex
                    ? InvalidNodeIndex
                    : hostRemap_[
                        static_cast<std::size_t>(
                            oldNode.left
                            )
                    ];

                copiedNode.right = InvalidNodeIndex;
                break;

            case NodeKind::BINARY:
                copiedNode.left =
                    oldNode.left == InvalidNodeIndex
                    ? InvalidNodeIndex
                    : hostRemap_[
                        static_cast<std::size_t>(
                            oldNode.left
                            )
                    ];

                copiedNode.right =
                    oldNode.right == InvalidNodeIndex
                    ? InvalidNodeIndex
                    : hostRemap_[
                        static_cast<std::size_t>(
                            oldNode.right
                            )
                    ];
                break;
            }

            extractedNodes[newPosition] =
                copiedNode;
        }

        return GPTree{
            std::move(extractedNodes),
            0
        };
    }

    GPTree GPTreeSubtreeOperations::graftSubtree(
        const GPTree& host,
        const NodeIndex replacedNode,
        const GPTree& donor)
    {
        if (host.isEmpty())
        {
            return host;
        }

        if (donor.isEmpty())
        {
            return host;
        }


        hostRemap_.assign(
            host.nodeCount(),
            InvalidNodeIndex
        );

        donorRemap_.assign(
            donor.nodeCount(),
            InvalidNodeIndex
        );

        GPTree::NodeContainer outputNodes;

        outputNodes.reserve(
            host.nodeCount() +
            donor.nodeCount()
        );

        const NodeIndex newRoot =
            cloneHostWithReplacement(
                host,
                host.rootIndex(),
                replacedNode,
                donor,
                outputNodes,
                hostRemap_,
                donorRemap_
            );

        return GPTree{
            std::move(outputNodes),
            newRoot
        };
    }
}