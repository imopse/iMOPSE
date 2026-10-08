#pragma once

#include "FeatureValues.hpp"
#include "GPTree.hpp"

namespace bntgp::gp
{
    [[nodiscard]]
    double evaluateTree(
        const GPTree& tree,
        const FeatureValuesView& featureValues
    ) noexcept;
}