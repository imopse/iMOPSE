#include "BNTGPParetoLineageLogger.hpp"

#include "utils/logger/CExperimentLogger.h"

#include <cstddef>
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
        std::string_view objectiveName(
            const BNTGPObjective objective) noexcept
        {
            switch (objective)
            {
            case BNTGPObjective::Makespan:
                return "makespan";

            case BNTGPObjective::Cost:
                return "cost";
            }

            return "unknown";
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

        [[nodiscard]]
        bool isStructuralMutation(
            const BNTGPStructuralMutationKind mutation) noexcept
        {
            return
                mutation ==
                BNTGPStructuralMutationKind::Structural;
        }

        [[nodiscard]]
        bool isMacroMutation(
            const BNTGPStructuralMutationKind mutation) noexcept
        {
            return
                mutation ==
                BNTGPStructuralMutationKind::MacroSubtree;
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

    BNTGPParetoLineageLogger::
        BNTGPParetoLineageLogger(
            ParetoLineageLoggerParameters parameters)
        : parameters_(std::move(parameters))
    {
    }

    BNTGPLogEventMask
        BNTGPParetoLineageLogger::subscribedEvents()
            const noexcept
    {
        return
            BNTGPLogEvent::RunStarted |
            BNTGPLogEvent::GenerationTrace |
            BNTGPLogEvent::RunCompleted;
    }

    void BNTGPParetoLineageLogger::onRunStarted(
        const BNTGPLoggingContext& context)
    {
        closeOutputs();

        if (CExperimentLogger::
            m_OutputDataPathPrefix.empty())
        {
            return;
        }

        const std::filesystem::path outputDirectory{
            CExperimentLogger::m_OutputDataPathPrefix
        };

        const bool lineageOpened = openBufferedOutput(
            lineageOutput_,
            lineageStreamBuffer_,
            outputDirectory / "pareto_lineage.csv",
            parameters_.streamBufferSizeBytes
        );

        if (!lineageOpened)
        {
            closeOutputs();
            return;
        }

        lineageOutput_
            << "generation,mating_id,child_slot,child_id,"
            << "parent1_id,parent2_id,"
            << "parent1_makespan,parent1_cost,"
            << "parent2_makespan,parent2_cost,"
            << "child_makespan,child_cost,"
            << "selected_objective,parent1_gap,parent2_gap,"
            << "crossover_selected,parameter_mutation_selected,"
            << "structural_mutation_selected,macro_mutation_selected,"
            << "tree_hash,tree_nodes,tree_depth,archive_status\n";

        if (parameters_.logArchiveDeltas)
        {
            const bool archiveOpened = openBufferedOutput(
                archiveOutput_,
                archiveStreamBuffer_,
                outputDirectory / "pareto_archive_delta.csv",
                parameters_.streamBufferSizeBytes
            );

            if (!archiveOpened)
            {
                closeOutputs();
                return;
            }

            archiveOutput_
                << "generation,event,individual_id,makespan,cost\n";
        }

        active_ = true;

        if (!parameters_.logArchiveDeltas)
        {
            return;
        }

        for (const BNTGPArchiveEntry& entry :
            context.archive.entries())
        {
            writeArchiveDelta(
                0U,
                BNTGPArchiveDeltaRecord{
                    BNTGPArchiveDeltaEvent::Added,
                    entry.individual().id(),
                    entry.individual().evaluation()
                }
            );
        }
    }

    void BNTGPParetoLineageLogger::onGenerationTrace(
        const BNTGPGenerationTrace& trace)
    {
        if (!active_)
        {
            return;
        }

        if (parameters_.logArchiveDeltas)
        {
            for (const BNTGPArchiveDeltaRecord& record :
                trace.archiveUpdate.deltaRecords)
            {
                writeArchiveDelta(
                    trace.generation,
                    record
                );
            }
        }

        const bool reachedSamplingGeneration =
            trace.generation %
            parameters_.samplingInterval == 0U;

        if (!reachedSamplingGeneration)
        {
            return;
        }

        for (const BNTGPOffspringLineageRecord& record :
            trace.offspring)
        {
            if (parameters_.acceptedOnly &&
                record.archiveStatus !=
                BNTGPArchiveCandidateStatus::Added)
            {
                continue;
            }

            writeLineageRecord(record);
        }
    }

    void BNTGPParetoLineageLogger::onRunCompleted(
        const BNTGPLoggingContext& context)
    {
        (void)context;
        closeOutputs();
    }

    void BNTGPParetoLineageLogger::closeOutputs() noexcept
    {
        if (lineageOutput_.is_open())
        {
            lineageOutput_.flush();
            lineageOutput_.close();
        }

        if (archiveOutput_.is_open())
        {
            archiveOutput_.flush();
            archiveOutput_.close();
        }

        lineageStreamBuffer_.clear();
        archiveStreamBuffer_.clear();

        active_ = false;
    }

    void BNTGPParetoLineageLogger::writeLineageRecord(
        const BNTGPOffspringLineageRecord& record)
    {
        lineageOutput_
            << record.generation << ','
            << record.matingId << ','
            << record.childSlot << ','
            << record.childId << ','
            << record.firstParentId << ','
            << record.secondParentId << ','
            << record.firstParentEvaluation.makespan << ','
            << record.firstParentEvaluation.cost << ','
            << record.secondParentEvaluation.makespan << ','
            << record.secondParentEvaluation.cost << ','
            << record.childEvaluation.makespan << ','
            << record.childEvaluation.cost << ','
            << objectiveName(record.selectedObjective) << ','
            << record.firstParentGap << ','
            << record.secondParentGap << ','
            << (record.crossoverSelected ? 1 : 0) << ','
            << (record.parameterMutationSelected ? 1 : 0) << ','
            << (isStructuralMutation(
                    record.structuralMutation) ? 1 : 0) << ','
            << (isMacroMutation(
                    record.structuralMutation) ? 1 : 0) << ','
            << record.treeIdentity << ','
            << record.treeNodeCount << ','
            << record.treeDepth << ','
            << archiveStatusName(record.archiveStatus)
            << '\n';
    }

    void BNTGPParetoLineageLogger::writeArchiveDelta(
        const std::size_t generation,
        const BNTGPArchiveDeltaRecord& record)
    {
        archiveOutput_
            << generation << ','
            << archiveDeltaEventName(record.event) << ','
            << record.individualId << ','
            << record.evaluation.makespan << ','
            << record.evaluation.cost
            << '\n';
    }
}
