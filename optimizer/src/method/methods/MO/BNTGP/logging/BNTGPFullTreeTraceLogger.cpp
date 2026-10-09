#include "BNTGPFullTreeTraceLogger.hpp"

#include "utils/logger/CExperimentLogger.h"
#include "../gp/FeatureCatalog.hpp"

#include <cstddef>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <limits>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace bntgp
{
    namespace
    {
        constexpr std::uint64_t SecondaryHashInitial =
            0x9ae16a3b2f90404fULL;

        constexpr std::uint64_t GoldenRatioConstant =
            0x9e3779b97f4a7c15ULL;

        [[nodiscard]]
        std::uint64_t mix64(std::uint64_t value) noexcept
        {
            value += GoldenRatioConstant;
            value = (value ^ (value >> 30U))
                * 0xbf58476d1ce4e5b9ULL;
            value = (value ^ (value >> 27U))
                * 0x94d049bb133111ebULL;
            return value ^ (value >> 31U);
        }

        void combineHash(
            std::uint64_t& seed,
            const std::uint64_t value) noexcept
        {
            seed ^= mix64(
                value + GoldenRatioConstant
                + (seed << 6U)
                + (seed >> 2U)
            );
        }

        [[nodiscard]]
        std::uint64_t doubleBits(const double value) noexcept
        {
            std::uint64_t bits = 0U;
            static_assert(sizeof(bits) == sizeof(value));
            std::memcpy(&bits, &value, sizeof(bits));
            return bits;
        }

        [[nodiscard]]
        std::uint64_t secondaryTreeHash(
            const gp::GPTree& tree) noexcept
        {
            std::uint64_t hash = SecondaryHashInitial;

            combineHash(
                hash,
                static_cast<std::uint64_t>(
                    static_cast<std::int64_t>(tree.rootIndex())
                )
            );

            combineHash(
                hash,
                static_cast<std::uint64_t>(tree.nodeCount())
            );

            for (const gp::GPNode& node : tree.nodes())
            {
                combineHash(hash, static_cast<std::uint64_t>(node.kind));
                combineHash(hash, static_cast<std::uint64_t>(node.left));
                combineHash(hash, static_cast<std::uint64_t>(node.right));
                combineHash(hash, static_cast<std::uint64_t>(node.feature));
                combineHash(hash, doubleBits(node.constant));
                combineHash(hash, static_cast<std::uint64_t>(node.binaryOperation));
                combineHash(hash, static_cast<std::uint64_t>(node.unaryOperation));
            }

            return hash;
        }

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
        std::string_view archiveDeltaName(
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
        std::string_view structuralMutationName(
            const BNTGPStructuralMutationKind mutation) noexcept
        {
            switch (mutation)
            {
            case BNTGPStructuralMutationKind::None:
                return "none";
            case BNTGPStructuralMutationKind::Structural:
                return "structural";
            case BNTGPStructuralMutationKind::MacroSubtree:
                return "macro_subtree";
            }

            return "unknown";
        }

        [[nodiscard]]
        std::string_view unaryOperationName(
            const gp::UnaryOp operation) noexcept
        {
            switch (operation)
            {
            case gp::UnaryOp::NEG:
                return "NEG";
            case gp::UnaryOp::ABS:
                return "ABS";
            }

            return "UNKNOWN_UNARY";
        }

        [[nodiscard]]
        std::string_view binaryOperationName(
            const gp::BinaryOp operation) noexcept
        {
            switch (operation)
            {
            case gp::BinaryOp::ADD:
                return "ADD";
            case gp::BinaryOp::SUB:
                return "SUB";
            case gp::BinaryOp::MUL:
                return "MUL";
            case gp::BinaryOp::DIV:
                return "DIV";
            case gp::BinaryOp::MIN:
                return "MIN";
            case gp::BinaryOp::MAX:
                return "MAX";
            }

            return "UNKNOWN_BINARY";
        }

        [[nodiscard]]
        std::string_view nodeKindName(
            const gp::NodeKind kind) noexcept
        {
            switch (kind)
            {
            case gp::NodeKind::CONST:
                return "CONST";
            case gp::NodeKind::FEATURE:
                return "FEATURE";
            case gp::NodeKind::UNARY:
                return "UNARY";
            case gp::NodeKind::BINARY:
                return "BINARY";
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

            output.open(path, std::ios::out | std::ios::trunc);

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

        void writeCsvQuoted(
            std::ostream& output,
            const std::string_view value)
        {
            output << '"';

            for (const char character : value)
            {
                if (character == '"')
                {
                    output << "\"\"";
                }
                else
                {
                    output << character;
                }
            }

            output << '"';
        }

        void appendTreeExpression(
            std::ostringstream& output,
            const gp::GPTree& tree,
            const gp::NodeIndex nodeIndex)
        {
            if (!tree.contains(nodeIndex))
            {
                output << "INVALID";
                return;
            }

            const gp::GPNode& node = tree.node(nodeIndex);

            switch (node.kind)
            {
            case gp::NodeKind::CONST:
                output << std::setprecision(
                    std::numeric_limits<double>::max_digits10
                ) << node.constant;
                return;

            case gp::NodeKind::FEATURE:
                output << gp::featureShortName(node.feature);
                return;

            case gp::NodeKind::UNARY:
                output << unaryOperationName(node.unaryOperation) << '(';
                appendTreeExpression(output, tree, node.left);
                output << ')';
                return;

            case gp::NodeKind::BINARY:
                output << binaryOperationName(node.binaryOperation) << '(';
                appendTreeExpression(output, tree, node.left);
                output << ',';
                appendTreeExpression(output, tree, node.right);
                output << ')';
                return;
            }

            output << "UNKNOWN_NODE";
        }

        [[nodiscard]]
        std::string treeExpression(const gp::GPTree& tree)
        {
            if (tree.isEmpty())
            {
                return "EMPTY";
            }

            std::ostringstream output;
            appendTreeExpression(output, tree, tree.rootIndex());
            return output.str();
        }

        [[nodiscard]]
        std::string exactNodeSerialization(const gp::GPTree& tree)
        {
            std::ostringstream output;

            for (std::size_t index = 0U;
                index < tree.nodeCount();
                ++index)
            {
                if (index != 0U)
                {
                    output << ';';
                }

                const gp::GPNode& node = tree.nodes()[index];

                output
                    << index << '|'
                    << nodeKindName(node.kind) << '|'
                    << doubleBits(node.constant) << '|'
                    << static_cast<unsigned>(node.feature) << '|'
                    << static_cast<unsigned>(node.unaryOperation) << '|'
                    << static_cast<unsigned>(node.binaryOperation) << '|'
                    << node.left << '|'
                    << node.right;
            }

            return output.str();
        }

        [[nodiscard]]
        bool archiveContains(
            const BNTGPArchive& archive,
            const BNTGPIndividualId individualId) noexcept
        {
            for (const BNTGPArchiveEntry& entry : archive.entries())
            {
                if (entry.individual().id() == individualId)
                {
                    return true;
                }
            }

            return false;
        }
    }

    BNTGPFullTreeTraceLogger::BNTGPFullTreeTraceLogger(
        FullTreeTraceLoggerParameters parameters)
        : parameters_(std::move(parameters))
    {
    }

    std::size_t BNTGPFullTreeTraceLogger::TreeFingerprintHasher::operator()(
        const TreeFingerprint& fingerprint) const noexcept
    {
        std::uint64_t hash = fingerprint.primary;
        combineHash(hash, fingerprint.secondary);
        combineHash(hash, static_cast<std::uint64_t>(fingerprint.nodeCount));
        combineHash(
            hash,
            static_cast<std::uint64_t>(
                static_cast<std::int64_t>(fingerprint.rootIndex)
            )
        );
        return static_cast<std::size_t>(hash);
    }

    BNTGPLogEventMask
        BNTGPFullTreeTraceLogger::subscribedEvents() const noexcept
    {
        return
            BNTGPLogEvent::RunStarted |
            BNTGPLogEvent::GenerationTrace |
            BNTGPLogEvent::RunCompleted;
    }

    void BNTGPFullTreeTraceLogger::onRunStarted(
        const BNTGPLoggingContext& context)
    {
        closeOutputs();

        if (CExperimentLogger::m_OutputDataPathPrefix.empty())
        {
            return;
        }

        const std::filesystem::path outputDirectory{
            CExperimentLogger::m_OutputDataPathPrefix
        };

        const std::size_t bufferSize =
            parameters_.streamBufferSizeBytes;

        const bool opened =
            openBufferedOutput(
                catalogOutput_,
                catalogBuffer_,
                outputDirectory / "tree_catalog.csv",
                bufferSize)
            && openBufferedOutput(
                offspringOutput_,
                offspringBuffer_,
                outputDirectory / "tree_offspring_trace.csv",
                bufferSize)
            && openBufferedOutput(
                archiveDeltaOutput_,
                archiveDeltaBuffer_,
                outputDirectory / "tree_archive_delta.csv",
                bufferSize)
            && openBufferedOutput(
                initialPopulationOutput_,
                initialPopulationBuffer_,
                outputDirectory / "tree_initial_population.csv",
                bufferSize)
            && openBufferedOutput(
                initialArchiveOutput_,
                initialArchiveBuffer_,
                outputDirectory / "tree_initial_archive.csv",
                bufferSize)
            && openBufferedOutput(
                finalPopulationOutput_,
                finalPopulationBuffer_,
                outputDirectory / "tree_final_population.csv",
                bufferSize)
            && openBufferedOutput(
                finalArchiveOutput_,
                finalArchiveBuffer_,
                outputDirectory / "tree_final_archive.csv",
                bufferSize);

        if (!opened)
        {
            closeOutputs();
            return;
        }

        treeCatalogIds_.clear();
        individualTreeIds_.clear();
        nextTreeCatalogId_ = 1U;

        treeCatalogIds_.reserve(
            context.population.size() * 128U
        );
        individualTreeIds_.reserve(
            context.population.size()
            * (context.configuredGenerationCount + 2U)
        );

        catalogOutput_
            << "tree_id,tree_hash,secondary_hash,root_index,node_count,"
            << "depth,expression,exact_nodes\n";

        offspringOutput_
            << "generation,mating_id,child_slot,child_id,base_parent_id,"
            << "parent1_id,parent2_id,parent1_tree_id,parent2_tree_id,"
            << "parent1_makespan,parent1_cost,"
            << "parent2_makespan,parent2_cost,selected_objective,"
            << "parent1_gap,parent2_gap,crossover_selected,"
            << "parameter_mutation_selected,structural_mutation,"
            << "base_tree_id,post_crossover_tree_id,"
            << "post_parameter_tree_id,final_tree_id,"
            << "crossover_changed,parameter_changed,structural_changed,"
            << "child_makespan,child_cost,archive_status\n";

        archiveDeltaOutput_
            << "generation,event,individual_id,makespan,cost,tree_id\n";

        const char* snapshotHeader =
            "individual_id,makespan,cost,tree_id,tree_hash,node_count,depth,";

        initialPopulationOutput_
            << snapshotHeader << "in_archive\n";
        finalPopulationOutput_
            << snapshotHeader << "in_archive\n";

        initialArchiveOutput_
            << "individual_id,makespan,cost,tree_id,tree_hash,node_count,depth\n";
        finalArchiveOutput_
            << "individual_id,makespan,cost,tree_id,tree_hash,node_count,depth\n";

        active_ = true;

        writePopulationSnapshot(
            initialPopulationOutput_,
            context.population,
            context.archive
        );

        writeArchiveSnapshot(
            initialArchiveOutput_,
            context.archive
        );

        for (const BNTGPArchiveEntry& entry : context.archive.entries())
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

    void BNTGPFullTreeTraceLogger::onGenerationTrace(
        const BNTGPGenerationTrace& trace)
    {
        if (!active_)
        {
            return;
        }

        for (const BNTGPOffspringLineageRecord& record : trace.offspring)
        {
            writeOffspringRecord(record);
        }

        for (const BNTGPArchiveDeltaRecord& record :
            trace.archiveUpdate.deltaRecords)
        {
            writeArchiveDelta(trace.generation, record);
        }
    }

    void BNTGPFullTreeTraceLogger::onRunCompleted(
        const BNTGPLoggingContext& context)
    {
        if (!active_)
        {
            return;
        }

        writePopulationSnapshot(
            finalPopulationOutput_,
            context.population,
            context.archive
        );

        writeArchiveSnapshot(
            finalArchiveOutput_,
            context.archive
        );

        closeOutputs();
    }

    void BNTGPFullTreeTraceLogger::closeOutputs() noexcept
    {
        std::ofstream* outputs[] = {
            &catalogOutput_,
            &offspringOutput_,
            &archiveDeltaOutput_,
            &initialPopulationOutput_,
            &initialArchiveOutput_,
            &finalPopulationOutput_,
            &finalArchiveOutput_
        };

        for (std::ofstream* output : outputs)
        {
            if (output->is_open())
            {
                output->flush();
                output->close();
            }
        }

        catalogBuffer_.clear();
        offspringBuffer_.clear();
        archiveDeltaBuffer_.clear();
        initialPopulationBuffer_.clear();
        initialArchiveBuffer_.clear();
        finalPopulationBuffer_.clear();
        finalArchiveBuffer_.clear();

        treeCatalogIds_.clear();
        individualTreeIds_.clear();
        nextTreeCatalogId_ = 1U;
        active_ = false;
    }

    BNTGPFullTreeTraceLogger::TreeCatalogId
        BNTGPFullTreeTraceLogger::registerTree(
            const gp::GPTree& tree)
    {
        const BNTGPEvaluationCacheKey primary =
            buildBNTGPEvaluationCacheKey(tree);

        const TreeFingerprint fingerprint{
            primary,
            secondaryTreeHash(tree),
            tree.nodeCount(),
            tree.rootIndex()
        };

        const auto existing = treeCatalogIds_.find(fingerprint);
        if (existing != treeCatalogIds_.end())
        {
            return existing->second;
        }

        const TreeCatalogId treeId = nextTreeCatalogId_++;
        treeCatalogIds_.emplace(fingerprint, treeId);

        catalogOutput_
            << treeId << ','
            << primary << ','
            << fingerprint.secondary << ','
            << tree.rootIndex() << ','
            << tree.nodeCount() << ','
            << tree.depth() << ',';

        const std::string expression = treeExpression(tree);
        writeCsvQuoted(catalogOutput_, expression);
        catalogOutput_ << ',';

        const std::string exactNodes = exactNodeSerialization(tree);
        writeCsvQuoted(catalogOutput_, exactNodes);
        catalogOutput_ << '\n';

        return treeId;
    }

    BNTGPFullTreeTraceLogger::TreeCatalogId
        BNTGPFullTreeTraceLogger::treeIdForIndividual(
            const BNTGPIndividualId individualId) const noexcept
    {
        const auto iterator = individualTreeIds_.find(individualId);
        return iterator == individualTreeIds_.end()
            ? 0U
            : iterator->second;
    }

    void BNTGPFullTreeTraceLogger::rememberIndividualTree(
        const BNTGPIndividualId individualId,
        const TreeCatalogId treeId)
    {
        if (individualId == InvalidBNTGPIndividualId || treeId == 0U)
        {
            return;
        }

        individualTreeIds_[individualId] = treeId;
    }

    void BNTGPFullTreeTraceLogger::writePopulationSnapshot(
        std::ofstream& output,
        const std::vector<BNTGPIndividual>& population,
        const BNTGPArchive& archive)
    {
        for (const BNTGPIndividual& individual : population)
        {
            const TreeCatalogId treeId = registerTree(individual.tree());
            rememberIndividualTree(individual.id(), treeId);

            const BNTGPEvaluation& evaluation = individual.evaluation();

            output
                << individual.id() << ','
                << evaluation.makespan << ','
                << evaluation.cost << ','
                << treeId << ','
                << buildBNTGPEvaluationCacheKey(individual.tree()) << ','
                << individual.tree().nodeCount() << ','
                << individual.tree().depth() << ','
                << (archiveContains(archive, individual.id()) ? 1 : 0)
                << '\n';
        }
    }

    void BNTGPFullTreeTraceLogger::writeArchiveSnapshot(
        std::ofstream& output,
        const BNTGPArchive& archive)
    {
        for (const BNTGPArchiveEntry& entry : archive.entries())
        {
            const BNTGPIndividual& individual = entry.individual();
            const TreeCatalogId treeId = registerTree(individual.tree());
            rememberIndividualTree(individual.id(), treeId);

            const BNTGPEvaluation& evaluation = individual.evaluation();

            output
                << individual.id() << ','
                << evaluation.makespan << ','
                << evaluation.cost << ','
                << treeId << ','
                << buildBNTGPEvaluationCacheKey(individual.tree()) << ','
                << individual.tree().nodeCount() << ','
                << individual.tree().depth() << '\n';
        }
    }

    void BNTGPFullTreeTraceLogger::writeArchiveDelta(
        const std::size_t generation,
        const BNTGPArchiveDeltaRecord& record)
    {
        archiveDeltaOutput_
            << generation << ','
            << archiveDeltaName(record.event) << ','
            << record.individualId << ','
            << record.evaluation.makespan << ','
            << record.evaluation.cost << ','
            << treeIdForIndividual(record.individualId)
            << '\n';
    }

    void BNTGPFullTreeTraceLogger::writeOffspringRecord(
        const BNTGPOffspringLineageRecord& record)
    {
        const BNTGPIndividualId baseParentId =
            record.childSlot == 0U
            ? record.firstParentId
            : record.secondParentId;

        const TreeCatalogId firstParentTreeId =
            treeIdForIndividual(record.firstParentId);

        const TreeCatalogId secondParentTreeId =
            treeIdForIndividual(record.secondParentId);

        const TreeCatalogId baseTreeId =
            record.childSlot == 0U
            ? firstParentTreeId
            : secondParentTreeId;

        TreeCatalogId postCrossoverTreeId = baseTreeId;

        if (record.postCrossoverTree.has_value())
        {
            postCrossoverTreeId =
                registerTree(*record.postCrossoverTree);
        }

        TreeCatalogId postParameterTreeId =
            postCrossoverTreeId;

        if (record.postParameterMutationTree.has_value())
        {
            postParameterTreeId =
                registerTree(*record.postParameterMutationTree);
        }

        TreeCatalogId finalTreeId = 0U;

        if (record.finalTree.has_value())
        {
            finalTreeId = registerTree(*record.finalTree);
            rememberIndividualTree(record.childId, finalTreeId);
        }

        const bool crossoverChanged =
            postCrossoverTreeId != baseTreeId;

        const bool parameterChanged =
            postParameterTreeId != postCrossoverTreeId;

        const bool structuralChanged =
            finalTreeId != 0U
            && finalTreeId != postParameterTreeId;

        offspringOutput_
            << record.generation << ','
            << record.matingId << ','
            << record.childSlot << ','
            << record.childId << ','
            << baseParentId << ','
            << record.firstParentId << ','
            << record.secondParentId << ','
            << firstParentTreeId << ','
            << secondParentTreeId << ','
            << record.firstParentEvaluation.makespan << ','
            << record.firstParentEvaluation.cost << ','
            << record.secondParentEvaluation.makespan << ','
            << record.secondParentEvaluation.cost << ','
            << objectiveName(record.selectedObjective) << ','
            << record.firstParentGap << ','
            << record.secondParentGap << ','
            << (record.crossoverSelected ? 1 : 0) << ','
            << (record.parameterMutationSelected ? 1 : 0) << ','
            << structuralMutationName(record.structuralMutation) << ','
            << baseTreeId << ','
            << postCrossoverTreeId << ','
            << postParameterTreeId << ','
            << finalTreeId << ','
            << (crossoverChanged ? 1 : 0) << ','
            << (parameterChanged ? 1 : 0) << ','
            << (structuralChanged ? 1 : 0) << ','
            << record.childEvaluation.makespan << ','
            << record.childEvaluation.cost << ','
            << archiveStatusName(record.archiveStatus)
            << '\n';
    }
}
