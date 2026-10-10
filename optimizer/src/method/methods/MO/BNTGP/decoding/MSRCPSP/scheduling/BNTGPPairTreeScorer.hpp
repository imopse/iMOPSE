#pragma once

#include "../../../gp/FeatureValues.hpp"
#include "../../../gp/GPTree.hpp"

#include <array>

#include "Features.hpp"

namespace bntgp::decoding::msrcpsp::scheduling
{
    class BNTGPPairTreeScorer final
    {
    public:
        explicit BNTGPPairTreeScorer(
            const gp::GPTree& tree
        ) noexcept;

        [[nodiscard]]
        bool usesFeature(
            gp::FeatureId feature
        ) const noexcept;

        [[nodiscard]]
        double score(
            const scheduling::Features& features
        ) noexcept;

    private:
        const gp::GPTree& tree_;

        std::array<
            bool,
            gp::FeatureCount
        > usedFeatures_{};

        gp::TaskFeatureValues taskValues_{};
        gp::ResourceFeatureValues resourceValues_{};
    };
}
