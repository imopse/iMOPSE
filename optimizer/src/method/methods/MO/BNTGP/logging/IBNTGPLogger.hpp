#pragma once

#include "../archive/BNTGPArchive.hpp"
#include "../core/BNTGPIndividual.hpp"
#include "../evolution/BNTGPGenerationTrace.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace bntgp
{
    struct BNTGPLoggingContext final
    {
        std::size_t generation{ 0U };
        std::size_t configuredGenerationCount{ 0U };

        const std::vector<BNTGPIndividual>& population;
        const BNTGPArchive& archive;
    };

    enum class BNTGPLogEvent : std::uint32_t
    {
        None = 0U,
        RunStarted = 1U << 0U,
        GenerationCompleted = 1U << 1U,
        RunCompleted = 1U << 2U,
        GenerationTrace = 1U << 3U
    };

    using BNTGPLogEventMask = std::uint32_t;

    [[nodiscard]]
    constexpr BNTGPLogEventMask toMask(
        const BNTGPLogEvent event) noexcept
    {
        return static_cast<BNTGPLogEventMask>(event);
    }

    [[nodiscard]]
    constexpr BNTGPLogEventMask operator|(
        const BNTGPLogEvent first,
        const BNTGPLogEvent second) noexcept
    {
        return toMask(first) | toMask(second);
    }

    [[nodiscard]]
    constexpr BNTGPLogEventMask operator|(
        const BNTGPLogEventMask mask,
        const BNTGPLogEvent event) noexcept
    {
        return mask | toMask(event);
    }

    class IBNTGPLogger
    {
    public:
        virtual ~IBNTGPLogger() = default;

        [[nodiscard]]
        virtual BNTGPLogEventMask subscribedEvents()
            const noexcept = 0;

        [[nodiscard]]
        virtual bool requiresTreeSnapshots() const noexcept
        {
            return false;
        }

        virtual void onRunStarted(
            const BNTGPLoggingContext& context)
        {
            (void)context;
        }

        virtual void onGenerationCompleted(
            const BNTGPLoggingContext& context)
        {
            (void)context;
        }

        virtual void onRunCompleted(
            const BNTGPLoggingContext& context)
        {
            (void)context;
        }

        virtual void onGenerationTrace(
            const BNTGPGenerationTrace& trace)
        {
            (void)trace;
        }
    };
}
