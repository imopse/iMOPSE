#include "CBNTGP.h"

#include "config/BNTGPConfigReader.hpp"
#include "evolution/BNTGPEvolutionEngine.hpp"

#include "decoding/MSRCPSP/BNTGPMSRCPSPDecoder.hpp"
#include "decoding/MSRCPSP/BNTGPMSRCPSPInstanceAdapter.hpp"

#include "problem/problems/MSRCPSP/CMSRCPSP_TA.h"

#include "utils/logger/CExperimentLogger.h"
#include "utils/random/CRandom.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace
{

    [[nodiscard]]
    CScheduler& resolveTaskAssignmentScheduler(
        AProblem& problem)
    {
        auto* taskAssignmentProblem =
            dynamic_cast<CMSRCPSP_TA*>(&problem);

        if (taskAssignmentProblem == nullptr)
        {
            throw std::runtime_error(
                "BNTGP works only with the MSRCPSP_TA2 problem."
            );
        }

        CScheduler* scheduler = taskAssignmentProblem->GetScheduler();
        if (scheduler == nullptr) {
            throw std::runtime_error("BNTGP received null MSRCPSP scheduler.");
        }
        return *scheduler;
    }

    [[nodiscard]]
    bntgp::BNTGPParameters readParameters(
        const SConfigMap* configuration)
    {
        if (configuration != nullptr)
        {
            return bntgp::ReadBNTGPConfiguration(
                *configuration
            );
        }

        const SConfigMap emptyConfiguration{};

        return bntgp::ReadBNTGPConfiguration(
            emptyConfiguration
        );
    }

    [[nodiscard]]
    std::uint64_t resolveSeed(
        const bntgp::BNTGPParameters& parameters)
    {
        if (parameters.seedOverride.has_value())
        {
            return *parameters.seedOverride;
        }

        return static_cast<std::uint64_t>(
            CRandom::GetSeed()
            );
    }

    void logFinalArchive(
        const bntgp::BNTGPArchive& archive)
    {
        std::vector<
            const bntgp::BNTGPArchiveEntry*
        > sortedEntries;

        sortedEntries.reserve(
            archive.size()
        );

        for (const bntgp::BNTGPArchiveEntry& entry :
            archive.entries())
        {
            sortedEntries.push_back(
                &entry
            );
        }

        std::sort(
            sortedEntries.begin(),
            sortedEntries.end(),
            [](
                const bntgp::BNTGPArchiveEntry* first,
                const bntgp::BNTGPArchiveEntry* second)
            {
                const bntgp::BNTGPEvaluation&
                    firstEvaluation =
                    first
                    ->individual()
                    .evaluation();

                const bntgp::BNTGPEvaluation&
                    secondEvaluation =
                    second
                    ->individual()
                    .evaluation();

                if (firstEvaluation.makespan !=
                    secondEvaluation.makespan)
                {
                    return
                        firstEvaluation.makespan
                        <
                        secondEvaluation.makespan;
                }

                return
                    firstEvaluation.cost
                    <
                    secondEvaluation.cost;
            }
        );

        std::ostringstream resultStream;

        for (const bntgp::BNTGPArchiveEntry* entry :
            sortedEntries)
        {
            const auto& current = entry->individual().evaluation();
            bool dominatedRaw = false;
            for (const auto* otherEntry : sortedEntries) {
                if (otherEntry == entry) continue;
                const auto& other = otherEntry->individual().evaluation();
                if (other.makespan <= current.makespan && other.cost <= current.cost &&
                    (other.makespan < current.makespan || other.cost < current.cost)) {
                    dominatedRaw = true;
                    break;
                }
            }
            if (dominatedRaw) continue;
            const bntgp::BNTGPEvaluation& evaluation =
                entry
                ->individual()
                .evaluation();

            resultStream
                << evaluation.makespan
                << ';'
                << evaluation.cost
                << '\n';
        }

        CExperimentLogger::LogResult(
            resultStream.str().c_str()
        );
    }
}

CBNTGP::CBNTGP(
    AProblem* problem,
    SConfigMap* const configuration)
    : problem_(problem),
    configuration_(configuration)
{
}

void CBNTGP::RunOptimization()
{
    CScheduler& scheduler =
        resolveTaskAssignmentScheduler(
            *problem_
        );

    bntgp::BNTGPParameters parameters =
        readParameters(
            configuration_
        );

    const std::uint64_t runSeed =
        resolveSeed(parameters);

    bntgp::decoding::msrcpsp::domain::Instance instance =
        bntgp::decoding::msrcpsp::fromScheduler(
            scheduler
        );

    auto decoder = std::make_unique<
        bntgp::decoding::msrcpsp::BNTGPMSRCPSPDecoder
    >(
        instance,
        scheduler,
        parameters.tree
    );

    bntgp::BNTGPEvolutionEngine engine{
        std::move(decoder),
        std::move(parameters),
        runSeed
    };

    engine.run();

    logFinalArchive(
        engine.archive()
    );
}