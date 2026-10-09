#pragma once

#include "../IBNTGPDecoder.hpp"
#include "../../config/BNTGPParameters.hpp"
#include "domain/Instance.hpp"
#include "scheduling/BNTGPScheduler.hpp"
#include "scheduling/FeatureScaling.hpp"
#include "scheduling/Precompute.hpp"

#include <optional>

class CScheduler;

namespace bntgp::decoding::msrcpsp
{
    class BNTGPMSRCPSPDecoder final : public IBNTGPDecoder
    {
    public:
        BNTGPMSRCPSPDecoder(
            const domain::Instance& sourceInstance,
            ::CScheduler& imopseScheduler,
            TreeParameters treeParameters
        );

        [[nodiscard]]
        BNTGPEvaluation decodeAndEvaluate(
            const gp::GPTree& tree
        ) override;

    private:
        [[nodiscard]]
        domain::Instance& resetWorkingInstance() noexcept;

        domain::Instance workingInstance_;
        scheduling::FeatureScaling scaling_{};
        scheduling::CPMPrecalc cpm_{};
        std::optional<scheduling::BNTGPScheduler> scheduler_{};
        ::CScheduler& imopseScheduler_;
        TreeParameters treeParameters_{};
    };
}
