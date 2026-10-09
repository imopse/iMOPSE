#pragma once

#include "../core/BNTGPIndividual.hpp"

#include <cassert>
#include <cstdint>

namespace bntgp
{
    enum class BNTGPObjective : std::uint8_t
    {
        Makespan = 0U,
        Cost = 1U
    };

    [[nodiscard]]
    inline double normalizedObjectiveValue(
        const BNTGPEvaluation& evaluation,
        const BNTGPObjective objective
    ) noexcept
    {
        switch (objective)
        {
        case BNTGPObjective::Makespan:
            return evaluation.normalizedMakespan;

        case BNTGPObjective::Cost:
            return evaluation.normalizedCost;
        }

        assert(false && "Unsupported BNTGP objective.");
        return 0.0;
    }

    [[nodiscard]]
    inline bool isDominatedBy(
        const BNTGPEvaluation& candidate,
        const BNTGPEvaluation& other
    ) noexcept
    {
        if (candidate.normalizedMakespan <
            other.normalizedMakespan)
        {
            return false;
        }

        if (candidate.normalizedCost <
            other.normalizedCost)
        {
            return false;
        }

        if (other.normalizedMakespan <
            candidate.normalizedMakespan)
        {
            return true;
        }

        if (other.normalizedCost <
            candidate.normalizedCost)
        {
            return true;
        }

        return false;
    }

    [[nodiscard]]
    inline bool hasDuplicateObjectiveValues(
        const BNTGPEvaluation& first,
        const BNTGPEvaluation& second
    ) noexcept
    {
        return
            first.normalizedMakespan ==
            second.normalizedMakespan
            &&
            first.normalizedCost ==
            second.normalizedCost;
    }
}