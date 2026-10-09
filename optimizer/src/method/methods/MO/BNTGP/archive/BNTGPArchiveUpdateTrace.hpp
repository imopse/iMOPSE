#pragma once

#include "../core/BNTGPIndividual.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace bntgp
{
    enum class BNTGPArchiveCandidateStatus : std::uint8_t
    {
        RejectedDominated = 0U,
        RejectedDuplicate = 1U,
        Added = 2U
    };

    enum class BNTGPArchiveDeltaEvent : std::uint8_t
    {
        Added = 0U,
        RemovedDominated = 1U
    };

    struct BNTGPArchiveDeltaRecord final
    {
        BNTGPArchiveDeltaEvent event{
            BNTGPArchiveDeltaEvent::Added
        };

        BNTGPIndividualId individualId{
            InvalidBNTGPIndividualId
        };

        BNTGPEvaluation evaluation{};
    };

    struct BNTGPArchiveUpdateTrace final
    {
        std::vector<BNTGPArchiveCandidateStatus>
            candidateStatuses{};

        std::vector<BNTGPArchiveDeltaRecord>
            deltaRecords{};

        void reset(
            const std::size_t candidateCount)
        {
            candidateStatuses.assign(
                candidateCount,
                BNTGPArchiveCandidateStatus::
                    RejectedDominated
            );

            deltaRecords.clear();
            deltaRecords.reserve(candidateCount * 2U);
        }
    };
}
