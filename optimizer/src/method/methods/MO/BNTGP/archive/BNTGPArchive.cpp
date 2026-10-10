#include "BNTGPArchive.hpp"

#include "BNTGPArchiveUpdateTrace.hpp"
#include "BNTGPObjectiveComparison.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <vector>

namespace bntgp
{
    namespace
    {
        [[nodiscard]]
        bool comesBeforeInCanonicalOrder(
            const BNTGPEvaluation& first,
            const BNTGPEvaluation& second
        ) noexcept
        {
            if (first.normalizedMakespan !=
                second.normalizedMakespan)
            {
                return
                    first.normalizedMakespan <
                    second.normalizedMakespan;
            }

            return
                first.normalizedCost <
                second.normalizedCost;
        }

#ifndef NDEBUG
        [[nodiscard]]
        bool isInCanonicalOrder(
            const BNTGPArchive::EntryContainer& entries
        ) noexcept
        {
            for (std::size_t index = 1U;
                index < entries.size();
                ++index)
            {
                const BNTGPEvaluation& previous =
                    entries[index - 1U]
                    .individual()
                    .evaluation();

                const BNTGPEvaluation& current =
                    entries[index]
                    .individual()
                    .evaluation();

                if (comesBeforeInCanonicalOrder(
                        current,
                        previous))
                {
                    return false;
                }
            }

            return true;
        }
#endif

        void recordCandidateStatus(
            BNTGPArchiveUpdateTrace* const updateTrace,
            const std::size_t candidateIndex,
            const BNTGPArchiveCandidateStatus status)
        {
            if (updateTrace == nullptr)
            {
                return;
            }

            assert(
                candidateIndex <
                updateTrace->candidateStatuses.size()
            );

            updateTrace->candidateStatuses[candidateIndex] =
                status;
        }

        void recordArchiveDelta(
            BNTGPArchiveUpdateTrace* const updateTrace,
            const BNTGPArchiveDeltaEvent event,
            const BNTGPIndividual& individual)
        {
            if (updateTrace == nullptr)
            {
                return;
            }

            updateTrace->deltaRecords.push_back({
                event,
                individual.id(),
                individual.evaluation()
            });
        }
    }

    bool BNTGPArchive::empty() const noexcept
    {
        return entries_.empty();
    }

    std::size_t BNTGPArchive::size() const noexcept
    {
        return entries_.size();
    }

    void BNTGPArchive::clear() noexcept
    {
        entries_.clear();
    }

    void BNTGPArchive::reserve(
        const std::size_t capacity)
    {
        entries_.reserve(capacity);
    }

    const BNTGPArchive::EntryContainer&
        BNTGPArchive::entries() const noexcept
    {
        return entries_;
    }

    BNTGPArchiveEntry& BNTGPArchive::entryAt(
        const std::size_t index) noexcept
    {
        assert(index < entries_.size());

        return entries_[index];
    }

    const BNTGPArchiveEntry&
        BNTGPArchive::entryAt(
            const std::size_t index) const noexcept
    {
        assert(index < entries_.size());

        return entries_[index];
    }

    void BNTGPArchive::updateWithCandidates(
        const std::vector<BNTGPIndividual>& candidates,
        BNTGPArchiveUpdateTrace* const updateTrace)
    {
#ifndef NDEBUG
        assert(isInCanonicalOrder(entries_));
#endif

        if (updateTrace != nullptr)
        {
            updateTrace->reset(candidates.size());
        }

        std::vector<std::size_t> acceptedCandidateIndices;
        acceptedCandidateIndices.reserve(candidates.size());

        for (std::size_t candidateIndex = 0U;
            candidateIndex < candidates.size();
            ++candidateIndex)
        {
            const BNTGPEvaluation& candidateEvaluation =
                candidates[candidateIndex].evaluation();

            bool rejected = false;
            bool duplicate = false;

            for (std::size_t otherIndex = 0U;
                !rejected && otherIndex < candidates.size();
                ++otherIndex)
            {
                if (candidateIndex == otherIndex)
                {
                    continue;
                }

                const BNTGPEvaluation& otherEvaluation =
                    candidates[otherIndex].evaluation();

                rejected = isDominatedBy(
                    candidateEvaluation,
                    otherEvaluation
                );

                if (!rejected &&
                    candidateIndex < otherIndex)
                {
                    duplicate = hasDuplicateObjectiveValues(
                        candidateEvaluation,
                        otherEvaluation
                    );

                    rejected = duplicate;
                }
            }

            for (std::size_t archiveIndex = 0U;
                !rejected && archiveIndex < entries_.size();
                ++archiveIndex)
            {
                const BNTGPEvaluation& archiveEvaluation =
                    entries_[archiveIndex]
                    .individual()
                    .evaluation();

                rejected = isDominatedBy(
                    candidateEvaluation,
                    archiveEvaluation
                );

                if (!rejected)
                {
                    duplicate = hasDuplicateObjectiveValues(
                        candidateEvaluation,
                        archiveEvaluation
                    );

                    rejected = duplicate;
                }
            }

            if (rejected)
            {
                recordCandidateStatus(
                    updateTrace,
                    candidateIndex,
                    duplicate
                    ? BNTGPArchiveCandidateStatus::
                        RejectedDuplicate
                    : BNTGPArchiveCandidateStatus::
                        RejectedDominated
                );

                continue;
            }

            acceptedCandidateIndices.push_back(
                candidateIndex
            );

            recordCandidateStatus(
                updateTrace,
                candidateIndex,
                BNTGPArchiveCandidateStatus::Added
            );
        }

        entries_.erase(
            std::remove_if(
                entries_.begin(),
                entries_.end(),
                [&](const BNTGPArchiveEntry& archiveEntry)
                {
                    const BNTGPEvaluation& archiveEvaluation =
                        archiveEntry
                        .individual()
                        .evaluation();

                    for (const std::size_t candidateIndex :
                        acceptedCandidateIndices)
                    {
                        const BNTGPEvaluation& candidateEvaluation =
                            candidates[candidateIndex]
                            .evaluation();

                        if (isDominatedBy(
                                archiveEvaluation,
                                candidateEvaluation))
                        {
                            recordArchiveDelta(
                                updateTrace,
                                BNTGPArchiveDeltaEvent::
                                    RemovedDominated,
                                archiveEntry.individual()
                            );

                            return true;
                        }
                    }

                    return false;
                }
            ),
            entries_.end()
        );

        entries_.reserve(
            entries_.size() +
            acceptedCandidateIndices.size()
        );

        for (const std::size_t candidateIndex :
            acceptedCandidateIndices)
        {
            const BNTGPIndividual& candidate =
                candidates[candidateIndex];

            const BNTGPEvaluation& candidateEvaluation =
                candidate.evaluation();

            const auto insertionPosition =
                std::lower_bound(
                    entries_.begin(),
                    entries_.end(),
                    candidateEvaluation,
                    [](
                        const BNTGPArchiveEntry& entry,
                        const BNTGPEvaluation& evaluation)
                    {
                        return comesBeforeInCanonicalOrder(
                            entry.individual().evaluation(),
                            evaluation
                        );
                    }
                );

            entries_.emplace(
                insertionPosition,
                candidate
            );

            recordArchiveDelta(
                updateTrace,
                BNTGPArchiveDeltaEvent::Added,
                candidate
            );
        }

#ifndef NDEBUG
        assert(isInCanonicalOrder(entries_));
#endif
    }
}
