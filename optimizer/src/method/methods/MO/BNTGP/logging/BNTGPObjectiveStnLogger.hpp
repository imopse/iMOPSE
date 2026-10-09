#pragma once

#include "../config/BNTGPParameters.hpp"
#include "IBNTGPLogger.hpp"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <unordered_map>
#include <vector>

namespace bntgp
{
    class BNTGPObjectiveStnLogger final
        : public IBNTGPLogger
    {
    public:
        explicit BNTGPObjectiveStnLogger(
            ObjectiveStnLoggerParameters parameters
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
        using StnNodeId = std::uint64_t;

        struct ObjectiveLocationKey final
        {
            int makespan{ 0 };
            std::uint64_t costBits{ 0U };

            [[nodiscard]]
            bool operator==(
                const ObjectiveLocationKey& other
            ) const noexcept
            {
                return
                    makespan == other.makespan &&
                    costBits == other.costBits;
            }
        };

        struct ObjectiveLocationKeyHash final
        {
            [[nodiscard]]
            std::size_t operator()(
                const ObjectiveLocationKey& key
            ) const noexcept;
        };

        struct StnNodeData final
        {
            StnNodeId id{ 0U };
            BNTGPEvaluation evaluation{};

            std::uint64_t visitCount{ 0U };
            std::uint64_t initialVisitCount{ 0U };
            std::uint64_t addedCount{ 0U };
            std::uint64_t dominatedCount{ 0U };
            std::uint64_t duplicateCount{ 0U };
            std::uint64_t archiveGenerationCount{ 0U };

            std::size_t firstGeneration{ 0U };
            std::size_t lastGeneration{ 0U };

            bool visited{ false };
            bool globalNondominated{ false };
        };

        struct ArchivePresenceState final
        {
            std::size_t activeIndividualCount{ 0U };
            std::size_t activeSinceGeneration{ 0U };
        };

        struct GenerationVisitData final
        {
            std::uint64_t visitCount{ 0U };
            std::uint64_t initialCount{ 0U };
            std::uint64_t addedCount{ 0U };
            std::uint64_t dominatedCount{ 0U };
            std::uint64_t duplicateCount{ 0U };
        };

        struct EdgeKey final
        {
            StnNodeId fromNodeId{ 0U };
            StnNodeId toNodeId{ 0U };

            bool crossoverSelected{ false };
            bool parameterMutationSelected{ false };
            bool structuralMutationSelected{ false };
            bool macroMutationSelected{ false };

            BNTGPArchiveCandidateStatus archiveStatus{
                BNTGPArchiveCandidateStatus::RejectedDominated
            };

            [[nodiscard]]
            bool operator==(
                const EdgeKey& other
            ) const noexcept;
        };

        struct EdgeKeyHash final
        {
            [[nodiscard]]
            std::size_t operator()(
                const EdgeKey& key
            ) const noexcept;
        };

        [[nodiscard]]
        static ObjectiveLocationKey buildLocationKey(
            const BNTGPEvaluation& evaluation
        ) noexcept;

        [[nodiscard]]
        StnNodeId nodeIdFor(
            const BNTGPEvaluation& evaluation
        );

        void recordVisit(
            std::size_t generation,
            const BNTGPEvaluation& evaluation,
            bool initial,
            BNTGPArchiveCandidateStatus archiveStatus
        );

        void recordTransition(
            std::size_t generation,
            const BNTGPEvaluation& parentEvaluation,
            const BNTGPOffspringLineageRecord& childRecord
        );

        void recordArchiveDelta(
            std::size_t generation,
            const BNTGPArchiveDeltaRecord& record
        );

        void finishArchivePresence(
            std::size_t completedGeneration
        );

        void flushGenerationVisits(
            std::size_t generation
        );

        void flushGenerationEdges(
            std::size_t generation
        );

        void writeNodes();
        void closeOutputs() noexcept;

        ObjectiveStnLoggerParameters parameters_{};
        bool active_{ false };

        std::unordered_map<
            ObjectiveLocationKey,
            StnNodeId,
            ObjectiveLocationKeyHash
        > nodeIdsByLocation_{};

        std::vector<StnNodeData> nodes_{};
        std::vector<ArchivePresenceState> archivePresence_{};

        std::unordered_map<
            StnNodeId,
            GenerationVisitData
        > generationVisits_{};

        std::unordered_map<
            EdgeKey,
            std::uint64_t,
            EdgeKeyHash
        > generationEdges_{};

        std::vector<char> nodeVisitStreamBuffer_{};
        std::vector<char> edgeStreamBuffer_{};
        std::vector<char> archiveDeltaStreamBuffer_{};

        std::ofstream nodeVisitOutput_{};
        std::ofstream edgeOutput_{};
        std::ofstream archiveDeltaOutput_{};
    };
}
