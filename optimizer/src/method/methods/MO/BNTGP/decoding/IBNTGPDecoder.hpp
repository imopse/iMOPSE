#pragma once

#include "../core/BNTGPIndividual.hpp"

namespace bntgp
{
    class IBNTGPDecoder
    {
    public:
        virtual ~IBNTGPDecoder() = default;

        [[nodiscard]]
        virtual BNTGPEvaluation decodeAndEvaluate(
            const gp::GPTree& tree
        ) = 0;
    };
}
