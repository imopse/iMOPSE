#pragma once

#include "BNTGPScheduleTypes.hpp"
#include "BNTGPSchedulingModel.hpp"

#include "../domain/Instance.hpp"
#include "../../../gp/GPTree.hpp"
#include "FeatureScaling.hpp"
#include "Precompute.hpp"

namespace bntgp::decoding::msrcpsp::scheduling
{
    class BNTGPScheduler final
    {
    public:
        BNTGPScheduler(
            const domain::Instance& instance,
            const scheduling::FeatureScaling& scaling,
            const scheduling::CPMPrecalc& cpm
        );

        [[nodiscard]]
        BNTGPScheduleResult withPairTree(
            domain::Instance& instance,
            const gp::GPTree& pairTree,
            const BNTGPScheduleOptions& options = {}
        ) const;

    private:
        FeatureScaling scaling_{};
        CPMPrecalc cpm_{};
        BNTGPSchedulingModel model_;
    };
}
