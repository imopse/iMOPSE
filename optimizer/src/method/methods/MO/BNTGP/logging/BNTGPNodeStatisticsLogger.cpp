#include "BNTGPNodeStatisticsLogger.hpp"

#include "../gp/FeatureCatalog.hpp"

#include "utils/logger/CExperimentLogger.h"

#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <utility>

namespace bntgp
{
    namespace
    {
        inline constexpr std::array<
            std::string_view,
            gp::FeatureCount
        > FeatureColumnNames{
            "DUR",
            "REQ",
            "AVAIL",
            "CRITLEN",
            "SLACK",
            "DESC_COUNT",
            "TASK_CRIT",
            "REL_PRESS",
            "GAP",
            "CHEAP",
            "CHEAP_PER_SK",
            "TASK_RES",
            "AVG_RES_COST",
            "UNSCHED",
            "MIN_COST_NOW",
            "REGRET_NOW",

            "RES_WAGE",
            "RES_SKILL",
            "RES_IDLE",
            "RES_CAN_NOW",
            "RES_UTIL",
            "RES_W_PER_L",
            "RES_ASSIGN_COST",
            "RES_PREMIUM",
            "RES_RESERVE",
            "RES_FAM_MIS",
            "RES_FUT_BRANCH",
            "RES_BOTTLENECK",
            "RES_SPEC_MIS",
            "RES_REL_WAGE"
        };

        static_assert(
            FeatureColumnNames.size() ==
            gp::FeatureSamplingOrder.size()
            );

        static_assert(
            gp::FeatureSamplingOrder[6] ==
            gp::FeatureId::TASK_CRITICAL_PRESSURE
            );

        static_assert(
            gp::FeatureSamplingOrder[7] ==
            gp::FeatureId::TASK_RELEASE_PRESSURE
            );

        struct NodeDistributionSnapshot final
        {
            std::size_t poolSize{ 0U };
            std::size_t totalNodeCount{ 0U };

            std::array<
                std::size_t,
                gp::FeatureCount
            > featureCounts{};
        };

        void accumulateTreeStatistics(
            const gp::GPTree& tree,
            NodeDistributionSnapshot& snapshot) noexcept
        {
            snapshot.totalNodeCount +=
                tree.nodeCount();

            for (const gp::GPNode& node :
                tree.nodes())
            {
                if (node.kind !=
                    gp::NodeKind::FEATURE)
                {
                    continue;
                }

                if (!gp::isValidFeature(
                    node.feature))
                {
                    continue;
                }

                const std::size_t featureIndex =
                    gp::toFeatureIndex(
                        node.feature
                    );

                ++snapshot
                    .featureCounts[featureIndex];
            }
        }

        [[nodiscard]]
        NodeDistributionSnapshot
            collectPopulationStatistics(
                const std::vector<BNTGPIndividual>& population)
        {
            NodeDistributionSnapshot snapshot{};
            snapshot.poolSize = population.size();

            for (const BNTGPIndividual& individual :
                population)
            {
                accumulateTreeStatistics(
                    individual.tree(),
                    snapshot
                );
            }

            return snapshot;
        }

        [[nodiscard]]
        NodeDistributionSnapshot
            collectArchiveStatistics(
                const BNTGPArchive& archive)
        {
            NodeDistributionSnapshot snapshot{};
            snapshot.poolSize = archive.size();

            for (const BNTGPArchiveEntry& entry :
                archive.entries())
            {
                accumulateTreeStatistics(
                    entry.individual().tree(),
                    snapshot
                );
            }

            return snapshot;
        }

        void writeSnapshotToFile(
            const NodeDistributionSnapshot& snapshot,
            const std::size_t generation,
            const char* const sourceName,
            const bool overwrite)
        {
            if (CExperimentLogger::
                m_OutputDataPathPrefix.empty())
            {
                return;
            }

            const std::filesystem::path outputPath =
                std::filesystem::path(
                    CExperimentLogger::
                    m_OutputDataPathPrefix
                )
                /
                (
                    "node_distribution_"
                    +
                    std::string(sourceName)
                    +
                    ".csv"
                    );

            std::ofstream output(
                outputPath,
                overwrite
                ? std::ios::out
                : (
                    std::ios::out
                    |
                    std::ios::app
                    )
            );

            if (!output.is_open())
            {
                return;
            }

            if (overwrite)
            {
                output
                    << "generation,pool_size,total_nodes";

                for (const std::string_view columnName :
                FeatureColumnNames)
                {
                    output
                        << ","
                        << columnName;
                }

                output << "\n";
            }

            output
                << generation
                << ","
                << snapshot.poolSize
                << ","
                << snapshot.totalNodeCount;

            for (const gp::FeatureId feature :
            gp::FeatureSamplingOrder)
            {
                output
                    << ","
                    << snapshot.featureCounts[
                        gp::toFeatureIndex(feature)
                    ];
            }

            output << "\n";
        }
    }

    BNTGPNodeStatisticsLogger::
        BNTGPNodeStatisticsLogger(
            NodeStatisticsLoggerParameters parameters) noexcept
        : parameters_(std::move(parameters))
    {
    }

    BNTGPLogEventMask
        BNTGPNodeStatisticsLogger::subscribedEvents()
            const noexcept
    {
        return
            BNTGPLogEvent::RunStarted |
            BNTGPLogEvent::GenerationCompleted;
    }

    void BNTGPNodeStatisticsLogger::onRunStarted(
        const BNTGPLoggingContext& context)
    {
        writeSnapshot(
            0U,
            true,
            context.population,
            context.archive
        );
    }

    void BNTGPNodeStatisticsLogger::onGenerationCompleted(
        const BNTGPLoggingContext& context)
    {
        const bool reachedSamplingStep =
            parameters_.samplingInterval > 0U
            &&
            (
                context.generation %
                parameters_.samplingInterval
                ) == 0U;

        const bool isFinalConfiguredGeneration =
            context.generation ==
            context.configuredGenerationCount;

        if (!reachedSamplingStep &&
            !isFinalConfiguredGeneration)
        {
            return;
        }

        writeSnapshot(
            context.generation,
            false,
            context.population,
            context.archive
        );
    }

    void BNTGPNodeStatisticsLogger::writeSnapshot(
        const std::size_t generation,
        const bool overwrite,
        const std::vector<BNTGPIndividual>& population,
        const BNTGPArchive& archive) const
    {
        if (parameters_.useArchive)
        {
            const NodeDistributionSnapshot snapshot =
                collectArchiveStatistics(
                    archive
                );

            writeSnapshotToFile(
                snapshot,
                generation,
                "archive",
                overwrite
            );

            return;
        }

        const NodeDistributionSnapshot snapshot =
            collectPopulationStatistics(
                population
            );

        writeSnapshotToFile(
            snapshot,
            generation,
            "population",
            overwrite
        );
    }
}