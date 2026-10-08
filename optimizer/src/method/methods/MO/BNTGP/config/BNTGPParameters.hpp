#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include "../gp/GPFeatureMask.hpp"

namespace bntgp
{
    struct EvolutionParameters final
    {
        std::size_t populationSize{ 50U };
        std::size_t generationCount{ 2000U };
        int tournamentSize{ 2 };
    };

    struct VariationParameters final
    {
        double crossoverProbability{ 0.60 };
        double parameterMutationProbability{ 0.01 };
        double structuralMutationProbability{ 0.01 };
    };

    struct TreeParameters final
    {
        int maximumDepth{ 8 };                                                                          
        bool softDepthEnabled{ true };
        int softDepthFreeEdges{ 9 };
        double softDepthMakespanPenalty{ 40.0 };
        double softDepthCostPenalty{ 1596.0 };
    };

    struct NodeStatisticsLoggerParameters final
    {
        bool enabled{ false };
        std::size_t samplingInterval{ 25U };
        bool useArchive{ false };
    };

    struct ParetoLineageLoggerParameters final
    {
        bool enabled{ false };

        std::size_t samplingInterval{ 1U };

        bool acceptedOnly{ false };

        bool logArchiveDeltas{ true };

        std::size_t streamBufferSizeBytes{ 1024U * 1024U };
    };

    struct ObjectiveStnLoggerParameters final
    {
        bool enabled{ false };

        std::size_t streamBufferSizeBytes{ 1024U * 1024U };
    };

    struct FullTreeTraceLoggerParameters final
    {
        bool enabled{ false };

        std::size_t streamBufferSizeBytes{ 8U * 1024U * 1024U };
    };

    struct LoggingParameters final
    {
        bool enabled{ true };

        NodeStatisticsLoggerParameters nodeStatistics{};
        ParetoLineageLoggerParameters paretoLineage{};
        ObjectiveStnLoggerParameters objectiveStn{};
        FullTreeTraceLoggerParameters fullTreeTrace{};
    };

    struct BNTGPParameters final
    {
        EvolutionParameters evolution{};
        VariationParameters variation{};
        TreeParameters tree{};
        gp::GPFeatureMask enabledFeatures{ gp::r23GPFeatures() };
        LoggingParameters logging{};

        std::optional<std::uint64_t> seedOverride{};
    };
}
