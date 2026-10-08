#pragma once

#include "../config/BNTGPParameters.hpp"
#include "IBNTGPLogger.hpp"

#include <cstddef>

namespace bntgp
{
    class BNTGPNodeStatisticsLogger final
        : public IBNTGPLogger
    {
    public:
        explicit BNTGPNodeStatisticsLogger(
            NodeStatisticsLoggerParameters parameters
        ) noexcept;

        [[nodiscard]]
        BNTGPLogEventMask subscribedEvents()
            const noexcept override;

        void onRunStarted(
            const BNTGPLoggingContext& context
        ) override;

        void onGenerationCompleted(
            const BNTGPLoggingContext& context
        ) override;

    private:
        void writeSnapshot(
            std::size_t generation,
            bool overwrite,
            const std::vector<BNTGPIndividual>& population,
            const BNTGPArchive& archive
        ) const;

        NodeStatisticsLoggerParameters parameters_{};
    };
}
