#pragma once

#include "IBNTGPLogger.hpp"
#include "../config/BNTGPParameters.hpp"
#include "../evaluation/BNTGPTreeIdentity.hpp"

#include <cstdint>
#include <fstream>
#include <unordered_map>
#include <vector>

namespace bntgp
{
    class BNTGPFullTreeTraceLogger final : public IBNTGPLogger
    {
    public:
        explicit BNTGPFullTreeTraceLogger(
            FullTreeTraceLoggerParameters parameters
        );

        [[nodiscard]]
        BNTGPLogEventMask subscribedEvents()
            const noexcept override;

        [[nodiscard]]
        bool requiresTreeSnapshots() const noexcept override
        {
            return true;
        }

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
        using TreeCatalogId = std::uint64_t;

        struct TreeFingerprint final
        {
            BNTGPEvaluationCacheKey primary{ 0U };
            std::uint64_t secondary{ 0U };
            std::size_t nodeCount{ 0U };
            gp::NodeIndex rootIndex{ gp::InvalidNodeIndex };

            [[nodiscard]]
            bool operator==(const TreeFingerprint& other) const noexcept
            {
                return primary == other.primary
                    && secondary == other.secondary
                    && nodeCount == other.nodeCount
                    && rootIndex == other.rootIndex;
            }
        };

        struct TreeFingerprintHasher final
        {
            [[nodiscard]]
            std::size_t operator()(
                const TreeFingerprint& fingerprint
            ) const noexcept;
        };

        void closeOutputs() noexcept;

        [[nodiscard]]
        TreeCatalogId registerTree(
            const gp::GPTree& tree
        );

        [[nodiscard]]
        TreeCatalogId treeIdForIndividual(
            BNTGPIndividualId individualId
        ) const noexcept;

        void rememberIndividualTree(
            BNTGPIndividualId individualId,
            TreeCatalogId treeId
        );

        void writePopulationSnapshot(
            std::ofstream& output,
            const std::vector<BNTGPIndividual>& population,
            const BNTGPArchive& archive
        );

        void writeArchiveSnapshot(
            std::ofstream& output,
            const BNTGPArchive& archive
        );

        void writeArchiveDelta(
            std::size_t generation,
            const BNTGPArchiveDeltaRecord& record
        );

        void writeOffspringRecord(
            const BNTGPOffspringLineageRecord& record
        );

        FullTreeTraceLoggerParameters parameters_{};
        bool active_{ false };

        TreeCatalogId nextTreeCatalogId_{ 1U };

        std::unordered_map<
            TreeFingerprint,
            TreeCatalogId,
            TreeFingerprintHasher
        > treeCatalogIds_{};

        std::unordered_map<
            BNTGPIndividualId,
            TreeCatalogId
        > individualTreeIds_{};

        std::ofstream catalogOutput_{};
        std::ofstream offspringOutput_{};
        std::ofstream archiveDeltaOutput_{};
        std::ofstream initialPopulationOutput_{};
        std::ofstream initialArchiveOutput_{};
        std::ofstream finalPopulationOutput_{};
        std::ofstream finalArchiveOutput_{};

        std::vector<char> catalogBuffer_{};
        std::vector<char> offspringBuffer_{};
        std::vector<char> archiveDeltaBuffer_{};
        std::vector<char> initialPopulationBuffer_{};
        std::vector<char> initialArchiveBuffer_{};
        std::vector<char> finalPopulationBuffer_{};
        std::vector<char> finalArchiveBuffer_{};
    };
}
