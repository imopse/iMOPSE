#include "BNTGPLoggerCollection.hpp"

#include <cassert>
#include <utility>

namespace bntgp
{
    bool BNTGPLoggerCollection::loggerSubscribesTo(
        const IBNTGPLogger& logger,
        const BNTGPLogEvent event) noexcept
    {
        return
            (logger.subscribedEvents() & toMask(event)) != 0U;
    }

    void BNTGPLoggerCollection::add(
        std::unique_ptr<IBNTGPLogger> logger)
    {
        assert(logger != nullptr);

        if (logger == nullptr)
        {
            return;
        }

        enabledEvents_ |= logger->subscribedEvents();
        treeSnapshotsRequired_ =
            treeSnapshotsRequired_ || logger->requiresTreeSnapshots();

        loggers_.push_back(
            std::move(logger)
        );
    }

    bool BNTGPLoggerCollection::empty() const noexcept
    {
        return loggers_.empty();
    }

    bool BNTGPLoggerCollection::isEnabled(
        const BNTGPLogEvent event) const noexcept
    {
        return (enabledEvents_ & toMask(event)) != 0U;
    }

    bool BNTGPLoggerCollection::requiresTreeSnapshots()
        const noexcept
    {
        return treeSnapshotsRequired_;
    }

    void BNTGPLoggerCollection::onRunStarted(
        const BNTGPLoggingContext& context)
    {
        for (const std::unique_ptr<IBNTGPLogger>& logger :
            loggers_)
        {
            if (loggerSubscribesTo(
                    *logger,
                    BNTGPLogEvent::RunStarted))
            {
                logger->onRunStarted(context);
            }
        }
    }

    void BNTGPLoggerCollection::onGenerationCompleted(
        const BNTGPLoggingContext& context)
    {
        for (const std::unique_ptr<IBNTGPLogger>& logger :
            loggers_)
        {
            if (loggerSubscribesTo(
                    *logger,
                    BNTGPLogEvent::GenerationCompleted))
            {
                logger->onGenerationCompleted(context);
            }
        }
    }

    void BNTGPLoggerCollection::onRunCompleted(
        const BNTGPLoggingContext& context)
    {
        for (const std::unique_ptr<IBNTGPLogger>& logger :
            loggers_)
        {
            if (loggerSubscribesTo(
                    *logger,
                    BNTGPLogEvent::RunCompleted))
            {
                logger->onRunCompleted(context);
            }
        }
    }

    void BNTGPLoggerCollection::onGenerationTrace(
        const BNTGPGenerationTrace& trace)
    {
        for (const std::unique_ptr<IBNTGPLogger>& logger :
            loggers_)
        {
            if (loggerSubscribesTo(
                    *logger,
                    BNTGPLogEvent::GenerationTrace))
            {
                logger->onGenerationTrace(trace);
            }
        }
    }
}
