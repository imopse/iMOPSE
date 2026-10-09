#include "BNTGPObjectiveStnLogger.hpp"

#include "utils/logger/CExperimentLogger.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <limits>
#include <string_view>
#include <utility>

namespace bntgp
{
    namespace
    {
        [[nodiscard]]
        std::size_t combineHash(
            const std::size_t seed,
            const std::size_t value) noexcept
        {
            return seed ^
                (value + 0x9e3779b9U + (seed << 6U) + (seed >> 2U));
        }

        [[nodiscard]]
        std::uint64_t doubleBits(
            const double value) noexcept
        {
            static_assert(
                sizeof(double) == sizeof(std::uint64_t),
                "BNTGP STN requires a 64-bit double representation."
            );

            std::uint64_t bits = 0U;
            std::memcpy(&bits, &value, sizeof(bits));
            return bits;
        }

        [[nodiscard]]
        bool isStructuralMutation(
            const BNTGPStructuralMutationKind mutation) noexcept
        {
            return mutation ==
                BNTGPStructuralMutationKind::Structural;
        }

        [[nodiscard]]
        bool isMacroMutation(
            const BNTGPStructuralMutationKind mutation) noexcept
        {
            return mutation ==
                BNTGPStructuralMutationKind::MacroSubtree;
        }

        [[nodiscard]]
        std::string_view archiveStatusName(
            const BNTGPArchiveCandidateStatus status) noexcept
        {
            switch (status)
            {
            case BNTGPArchiveCandidateStatus::Added:
                return "added";

            case BNTGPArchiveCandidateStatus::RejectedDuplicate:
                return "duplicate";

            case BNTGPArchiveCandidateStatus::RejectedDominated:
                return "dominated";
            }

            return "unknown";
        }

        [[nodiscard]]
        std::string_view archiveDeltaEventName(
            const BNTGPArchiveDeltaEvent event) noexcept
        {
            switch (event)
            {
            case BNTGPArchiveDeltaEvent::Added:
                return "ADD";

            case BNTGPArchiveDeltaEvent::RemovedDominated:
                return "REMOVE";
            }

            return "UNKNOWN";
        }

        bool openBufferedOutput(
            std::ofstream& output,
            std::vector<char>& buffer,
            const std::filesystem::path& path,
            const std::size_t bufferSizeBytes)
        {
            buffer.assign(bufferSizeBytes, '\0');

            output.rdbuf()->pubsetbuf(
                buffer.data(),
                static_cast<std::streamsize>(buffer.size())
            );

            output.open(
                path,
                std::ios::out | std::ios::trunc
            );

            if (!output.is_open())
            {
                buffer.clear();
                return false;
            }

            output << std::setprecision(
                std::numeric_limits<double>::max_digits10
            );

            return true;
        }
    }

    BNTGPObjectiveStnLogger::BNTGPObjectiveStnLogger(
        ObjectiveStnLoggerParameters parameters)
        : parameters_(std::move(parameters))
    {
    }

    BNTGPLogEventMask
        BNTGPObjectiveStnLogger::subscribedEvents()
            const noexcept
    {
        return
            BNTGPLogEvent::RunStarted |
            BNTGPLogEvent::GenerationTrace |
            BNTGPLogEvent::RunCompleted;
    }

    std::size_t
        BNTGPObjectiveStnLogger::ObjectiveLocationKeyHash::operator()(
            const ObjectiveLocationKey& key) const noexcept
    {
        std::size_t hash = std::hash<int>{}(key.makespan);
        hash = combineHash(
            hash,
            std::hash<std::uint64_t>{}(key.costBits)
        );
        return hash;
    }

    bool BNTGPObjectiveStnLogger::EdgeKey::operator==(
        const EdgeKey& other) const noexcept
    {
        return
            fromNodeId == other.fromNodeId &&
            toNodeId == other.toNodeId &&
            crossoverSelected == other.crossoverSelected &&
            parameterMutationSelected ==
                other.parameterMutationSelected &&
            structuralMutationSelected ==
                other.structuralMutationSelected &&
            macroMutationSelected == other.macroMutationSelected &&
            archiveStatus == other.archiveStatus;
    }

    std::size_t BNTGPObjectiveStnLogger::EdgeKeyHash::operator()(
        const EdgeKey& key) const noexcept
    {
        std::size_t hash = std::hash<StnNodeId>{}(key.fromNodeId);
        hash = combineHash(
            hash,
            std::hash<StnNodeId>{}(key.toNodeId)
        );
        hash = combineHash(hash, key.crossoverSelected ? 1U : 0U);
        hash = combineHash(
            hash,
            key.parameterMutationSelected ? 1U : 0U
        );
        hash = combineHash(
            hash,
            key.structuralMutationSelected ? 1U : 0U
        );
        hash = combineHash(
            hash,
            key.macroMutationSelected ? 1U : 0U
        );
        hash = combineHash(
            hash,
            static_cast<std::size_t>(key.archiveStatus)
        );
        return hash;
    }

    BNTGPObjectiveStnLogger::ObjectiveLocationKey
        BNTGPObjectiveStnLogger::buildLocationKey(
            const BNTGPEvaluation& evaluation) noexcept
    {
        return ObjectiveLocationKey{
            evaluation.makespan,
            doubleBits(evaluation.cost)
        };
    }

    BNTGPObjectiveStnLogger::StnNodeId
        BNTGPObjectiveStnLogger::nodeIdFor(
            const BNTGPEvaluation& evaluation)
    {
        const ObjectiveLocationKey key =
            buildLocationKey(evaluation);

        const auto existing = nodeIdsByLocation_.find(key);
        if (existing != nodeIdsByLocation_.end())
        {
            return existing->second;
        }

        const StnNodeId nodeId =
            static_cast<StnNodeId>(nodes_.size() + 1U);

        StnNodeData node{};
        node.id = nodeId;
        node.evaluation = evaluation;

        nodeIdsByLocation_.emplace(key, nodeId);
        nodes_.push_back(std::move(node));
        archivePresence_.push_back(ArchivePresenceState{});

        return nodeId;
    }

    void BNTGPObjectiveStnLogger::onRunStarted(
        const BNTGPLoggingContext& context)
    {
        closeOutputs();

        nodeIdsByLocation_.clear();
        nodes_.clear();
        archivePresence_.clear();
        generationVisits_.clear();
        generationEdges_.clear();

        if (CExperimentLogger::m_OutputDataPathPrefix.empty())
        {
            return;
        }

        const std::filesystem::path outputDirectory{
            CExperimentLogger::m_OutputDataPathPrefix
        };

        const bool visitsOpened = openBufferedOutput(
            nodeVisitOutput_,
            nodeVisitStreamBuffer_,
            outputDirectory / "stn_node_visits.csv",
            parameters_.streamBufferSizeBytes
        );

        const bool edgesOpened = openBufferedOutput(
            edgeOutput_,
            edgeStreamBuffer_,
            outputDirectory / "stn_edges.csv",
            parameters_.streamBufferSizeBytes
        );

        const bool archiveOpened = openBufferedOutput(
            archiveDeltaOutput_,
            archiveDeltaStreamBuffer_,
            outputDirectory / "stn_archive_delta.csv",
            parameters_.streamBufferSizeBytes
        );

        if (!visitsOpened || !edgesOpened || !archiveOpened)
        {
            closeOutputs();
            return;
        }

        nodeVisitOutput_
            << "generation,node_id,visit_count,initial_count,"
            << "added_count,dominated_count,duplicate_count\n";

        edgeOutput_
            << "generation,from_node_id,to_node_id,transition_count,"
            << "crossover_selected,parameter_mutation_selected,"
            << "structural_mutation_selected,macro_mutation_selected,"
            << "archive_status\n";

        archiveDeltaOutput_
            << "generation,event,node_id,individual_id,makespan,cost\n";

        active_ = true;

        nodeIdsByLocation_.reserve(
            context.population.size() * 4U
        );
        generationVisits_.reserve(context.population.size());
        generationEdges_.reserve(context.population.size() * 2U);

        for (const BNTGPIndividual& individual :
            context.population)
        {
            recordVisit(
                0U,
                individual.evaluation(),
                true,
                BNTGPArchiveCandidateStatus::RejectedDominated
            );
        }

        flushGenerationVisits(0U);

        for (const BNTGPArchiveEntry& entry :
            context.archive.entries())
        {
            recordArchiveDelta(
                0U,
                BNTGPArchiveDeltaRecord{
                    BNTGPArchiveDeltaEvent::Added,
                    entry.individual().id(),
                    entry.individual().evaluation()
                }
            );
        }
    }

    void BNTGPObjectiveStnLogger::onGenerationTrace(
        const BNTGPGenerationTrace& trace)
    {
        if (!active_)
        {
            return;
        }

        generationVisits_.clear();
        generationEdges_.clear();

        for (const BNTGPArchiveDeltaRecord& delta :
            trace.archiveUpdate.deltaRecords)
        {
            recordArchiveDelta(trace.generation, delta);
        }

        for (const BNTGPOffspringLineageRecord& record :
            trace.offspring)
        {
            recordVisit(
                trace.generation,
                record.childEvaluation,
                false,
                record.archiveStatus
            );

            if (record.crossoverSelected)
            {
                recordTransition(
                    trace.generation,
                    record.firstParentEvaluation,
                    record
                );

                recordTransition(
                    trace.generation,
                    record.secondParentEvaluation,
                    record
                );
            }
            else if (record.childSlot == 0U)
            {
                recordTransition(
                    trace.generation,
                    record.firstParentEvaluation,
                    record
                );
            }
            else
            {
                recordTransition(
                    trace.generation,
                    record.secondParentEvaluation,
                    record
                );
            }
        }

        flushGenerationVisits(trace.generation);
        flushGenerationEdges(trace.generation);
    }

    void BNTGPObjectiveStnLogger::onRunCompleted(
        const BNTGPLoggingContext& context)
    {
        if (!active_)
        {
            return;
        }

        finishArchivePresence(context.generation);

        for (const BNTGPArchiveEntry& entry :
            context.archive.entries())
        {
            const StnNodeId nodeId =
                nodeIdFor(entry.individual().evaluation());

            nodes_[static_cast<std::size_t>(nodeId - 1U)]
                .globalNondominated = true;
        }

        writeNodes();
        closeOutputs();
    }

    void BNTGPObjectiveStnLogger::recordVisit(
        const std::size_t generation,
        const BNTGPEvaluation& evaluation,
        const bool initial,
        const BNTGPArchiveCandidateStatus archiveStatus)
    {
        const StnNodeId nodeId = nodeIdFor(evaluation);
        StnNodeData& node =
            nodes_[static_cast<std::size_t>(nodeId - 1U)];

        ++node.visitCount;

        if (!node.visited)
        {
            node.firstGeneration = generation;
            node.visited = true;
        }

        node.lastGeneration = generation;

        GenerationVisitData& generationVisit =
            generationVisits_[nodeId];

        ++generationVisit.visitCount;

        if (initial)
        {
            ++node.initialVisitCount;
            ++generationVisit.initialCount;
            return;
        }

        switch (archiveStatus)
        {
        case BNTGPArchiveCandidateStatus::Added:
            ++node.addedCount;
            ++generationVisit.addedCount;
            break;

        case BNTGPArchiveCandidateStatus::RejectedDuplicate:
            ++node.duplicateCount;
            ++generationVisit.duplicateCount;
            break;

        case BNTGPArchiveCandidateStatus::RejectedDominated:
            ++node.dominatedCount;
            ++generationVisit.dominatedCount;
            break;
        }
    }

    void BNTGPObjectiveStnLogger::recordTransition(
        const std::size_t generation,
        const BNTGPEvaluation& parentEvaluation,
        const BNTGPOffspringLineageRecord& childRecord)
    {
        const StnNodeId fromNodeId =
            nodeIdFor(parentEvaluation);

        const StnNodeId toNodeId =
            nodeIdFor(childRecord.childEvaluation);

        const EdgeKey key{
            fromNodeId,
            toNodeId,
            childRecord.crossoverSelected,
            childRecord.parameterMutationSelected,
            isStructuralMutation(childRecord.structuralMutation),
            isMacroMutation(childRecord.structuralMutation),
            childRecord.archiveStatus
        };

        ++generationEdges_[key];
        (void)generation;
    }

    void BNTGPObjectiveStnLogger::recordArchiveDelta(
        const std::size_t generation,
        const BNTGPArchiveDeltaRecord& record)
    {
        const StnNodeId nodeId = nodeIdFor(record.evaluation);
        ArchivePresenceState& state =
            archivePresence_[static_cast<std::size_t>(nodeId - 1U)];

        if (record.event == BNTGPArchiveDeltaEvent::Added)
        {
            if (state.activeIndividualCount == 0U)
            {
                state.activeSinceGeneration = generation;
            }

            ++state.activeIndividualCount;
        }
        else if (state.activeIndividualCount > 0U)
        {
            --state.activeIndividualCount;

            if (state.activeIndividualCount == 0U)
            {
                StnNodeData& node =
                    nodes_[static_cast<std::size_t>(nodeId - 1U)];

                node.archiveGenerationCount +=
                    static_cast<std::uint64_t>(
                        generation - state.activeSinceGeneration
                    );
            }
        }

        archiveDeltaOutput_
            << generation << ','
            << archiveDeltaEventName(record.event) << ','
            << nodeId << ','
            << record.individualId << ','
            << record.evaluation.makespan << ','
            << record.evaluation.cost
            << '\n';
    }

    void BNTGPObjectiveStnLogger::finishArchivePresence(
        const std::size_t completedGeneration)
    {
        const std::size_t nodeCount = nodes_.size();

        for (std::size_t index = 0U;
            index < nodeCount;
            ++index)
        {
            ArchivePresenceState& state = archivePresence_[index];

            if (state.activeIndividualCount == 0U)
            {
                continue;
            }

            nodes_[index].archiveGenerationCount +=
                static_cast<std::uint64_t>(
                    completedGeneration -
                    state.activeSinceGeneration + 1U
                );

            state.activeIndividualCount = 0U;
        }
    }

    void BNTGPObjectiveStnLogger::flushGenerationVisits(
        const std::size_t generation)
    {
        std::vector<StnNodeId> nodeIds{};
        nodeIds.reserve(generationVisits_.size());

        for (const auto& entry : generationVisits_)
        {
            nodeIds.push_back(entry.first);
        }

        std::sort(nodeIds.begin(), nodeIds.end());

        for (const StnNodeId nodeId : nodeIds)
        {
            const GenerationVisitData& data =
                generationVisits_.at(nodeId);

            nodeVisitOutput_
                << generation << ','
                << nodeId << ','
                << data.visitCount << ','
                << data.initialCount << ','
                << data.addedCount << ','
                << data.dominatedCount << ','
                << data.duplicateCount
                << '\n';
        }

        generationVisits_.clear();
    }

    void BNTGPObjectiveStnLogger::flushGenerationEdges(
        const std::size_t generation)
    {
        using EdgeEntry = std::pair<EdgeKey, std::uint64_t>;

        std::vector<EdgeEntry> edges{};
        edges.reserve(generationEdges_.size());

        for (const auto& entry : generationEdges_)
        {
            edges.emplace_back(entry.first, entry.second);
        }

        std::sort(
            edges.begin(),
            edges.end(),
            [](const EdgeEntry& first, const EdgeEntry& second)
            {
                const EdgeKey& left = first.first;
                const EdgeKey& right = second.first;

                if (left.fromNodeId != right.fromNodeId)
                {
                    return left.fromNodeId < right.fromNodeId;
                }
                if (left.toNodeId != right.toNodeId)
                {
                    return left.toNodeId < right.toNodeId;
                }
                if (left.crossoverSelected != right.crossoverSelected)
                {
                    return left.crossoverSelected < right.crossoverSelected;
                }
                if (left.parameterMutationSelected !=
                    right.parameterMutationSelected)
                {
                    return left.parameterMutationSelected <
                        right.parameterMutationSelected;
                }
                if (left.structuralMutationSelected !=
                    right.structuralMutationSelected)
                {
                    return left.structuralMutationSelected <
                        right.structuralMutationSelected;
                }
                if (left.macroMutationSelected !=
                    right.macroMutationSelected)
                {
                    return left.macroMutationSelected <
                        right.macroMutationSelected;
                }

                return static_cast<std::uint8_t>(left.archiveStatus) <
                    static_cast<std::uint8_t>(right.archiveStatus);
            }
        );

        for (const EdgeEntry& entry : edges)
        {
            const EdgeKey& key = entry.first;

            edgeOutput_
                << generation << ','
                << key.fromNodeId << ','
                << key.toNodeId << ','
                << entry.second << ','
                << (key.crossoverSelected ? 1 : 0) << ','
                << (key.parameterMutationSelected ? 1 : 0) << ','
                << (key.structuralMutationSelected ? 1 : 0) << ','
                << (key.macroMutationSelected ? 1 : 0) << ','
                << archiveStatusName(key.archiveStatus)
                << '\n';
        }

        generationEdges_.clear();
    }

    void BNTGPObjectiveStnLogger::writeNodes()
    {
        if (CExperimentLogger::m_OutputDataPathPrefix.empty())
        {
            return;
        }

        const std::filesystem::path outputPath =
            std::filesystem::path{
                CExperimentLogger::m_OutputDataPathPrefix
            } / "stn_nodes.csv";

        std::ofstream output{
            outputPath,
            std::ios::out | std::ios::trunc
        };

        if (!output.is_open())
        {
            return;
        }

        output << std::setprecision(
            std::numeric_limits<double>::max_digits10
        );

        output
            << "node_id,makespan,cost,total_visit_count,"
            << "initial_visit_count,added_count,dominated_count,"
            << "duplicate_count,archive_generation_count,"
            << "first_generation,last_generation,"
            << "global_nondominated\n";

        for (const StnNodeData& node : nodes_)
        {
            output
                << node.id << ','
                << node.evaluation.makespan << ','
                << node.evaluation.cost << ','
                << node.visitCount << ','
                << node.initialVisitCount << ','
                << node.addedCount << ','
                << node.dominatedCount << ','
                << node.duplicateCount << ','
                << node.archiveGenerationCount << ','
                << node.firstGeneration << ','
                << node.lastGeneration << ','
                << (node.globalNondominated ? 1 : 0)
                << '\n';
        }
    }

    void BNTGPObjectiveStnLogger::closeOutputs() noexcept
    {
        if (nodeVisitOutput_.is_open())
        {
            nodeVisitOutput_.flush();
            nodeVisitOutput_.close();
        }

        if (edgeOutput_.is_open())
        {
            edgeOutput_.flush();
            edgeOutput_.close();
        }

        if (archiveDeltaOutput_.is_open())
        {
            archiveDeltaOutput_.flush();
            archiveDeltaOutput_.close();
        }

        nodeVisitStreamBuffer_.clear();
        edgeStreamBuffer_.clear();
        archiveDeltaStreamBuffer_.clear();

        active_ = false;
    }
}
