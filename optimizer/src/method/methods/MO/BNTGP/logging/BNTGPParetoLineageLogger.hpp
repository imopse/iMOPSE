#pragma once

#include "../config/BNTGPParameters.hpp"
#include "IBNTGPLogger.hpp"

#include <fstream>
#include <vector>

namespace bntgp
{
    class BNTGPParetoLineageLogger final
        : public IBNTGPLogger
    {
    public:
        explicit BNTGPParetoLineageLogger(
            ParetoLineageLoggerParameters parameters
        );

        [[nodiscard]]
        BNTGPLogEventMask subscribedEvents()
            const noexcept override;

        void onRunStarted(
            const BNTGPLoggingContext& context
        ) override;

        void onGenerationTrace(
            const BNTGPGenerationTrace& trace
        ) override;

        void onRunCompleted(
            const BNTGPLoggingContext& context
        ) override;

    private:
        void closeOutputs() noexcept;

        void writeLineageRecord(
            const BNTGPOffspringLineageRecord& record
        );

        void writeArchiveDelta(
            std::size_t generation,
            const BNTGPArchiveDeltaRecord& record
        );

        ParetoLineageLoggerParameters parameters_{};

        bool active_{ false };

        std::vector<char> lineageStreamBuffer_{};
        std::vector<char> archiveStreamBuffer_{};

        std::ofstream lineageOutput_{};
        std::ofstream archiveOutput_{};
    };
}
