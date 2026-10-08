#include "BNTGPLoggerFactory.hpp"

#include "BNTGPNodeStatisticsLogger.hpp"
#include "BNTGPFullTreeTraceLogger.hpp"
#include "BNTGPObjectiveStnLogger.hpp"
#include "BNTGPParetoLineageLogger.hpp"

#include <memory>

namespace bntgp
{
    BNTGPLoggerCollection createBNTGPLoggers(
        const LoggingParameters& parameters)
    {
        BNTGPLoggerCollection loggers{};

        if (!parameters.enabled)
        {
            return loggers;
        }

        if (parameters.nodeStatistics.enabled)
        {
            loggers.add(
                std::make_unique<
                    BNTGPNodeStatisticsLogger
                >(
                    parameters.nodeStatistics
                )
            );
        }

        if (parameters.paretoLineage.enabled)
        {
            loggers.add(
                std::make_unique<
                    BNTGPParetoLineageLogger
                >(
                    parameters.paretoLineage
                )
            );
        }

        if (parameters.objectiveStn.enabled)
        {
            loggers.add(
                std::make_unique<
                    BNTGPObjectiveStnLogger
                >(
                    parameters.objectiveStn
                )
            );
        }

        if (parameters.fullTreeTrace.enabled)
        {
            loggers.add(
                std::make_unique<
                    BNTGPFullTreeTraceLogger
                >(
                    parameters.fullTreeTrace
                )
            );
        }

        return loggers;
    }
}
