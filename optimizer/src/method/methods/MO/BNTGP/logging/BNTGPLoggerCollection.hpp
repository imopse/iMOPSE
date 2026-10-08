#pragma once

#include "IBNTGPLogger.hpp"

#include <memory>
#include <vector>

namespace bntgp
{
    class BNTGPLoggerCollection final
    {
    public:
        void add(
            std::unique_ptr<IBNTGPLogger> logger
        );

        [[nodiscard]]
        bool empty() const noexcept;

        [[nodiscard]]
        bool isEnabled(
            BNTGPLogEvent event
        ) const noexcept;

        [[nodiscard]]
        bool requiresTreeSnapshots() const noexcept;

        void onRunStarted(
            const BNTGPLoggingContext& context
        );

        void onGenerationCompleted(
            const BNTGPLoggingContext& context
        );

        void onRunCompleted(
            const BNTGPLoggingContext& context
        );

        void onGenerationTrace(
            const BNTGPGenerationTrace& trace
        );

    private:
        [[nodiscard]]
        static bool loggerSubscribesTo(
            const IBNTGPLogger& logger,
            BNTGPLogEvent event
        ) noexcept;

        BNTGPLogEventMask enabledEvents_{ 0U };
        bool treeSnapshotsRequired_{ false };

        std::vector<
            std::unique_ptr<IBNTGPLogger>
        > loggers_{};
    };
}
