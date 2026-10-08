#include "BNTGPGapSelection.hpp"

#include "../../archive/BNTGPObjectiveComparison.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <limits>
#include <random>

namespace bntgp
{
    namespace
    {
        [[nodiscard]]
        int randomInt(
            std::mt19937& randomEngine,
            const int minimum,
            const int maximum)
        {
            std::uniform_int_distribution<int> distribution(
                minimum,
                maximum
            );

            return distribution(randomEngine);
        }

        [[nodiscard]]
        std::size_t requiredParentPairCount(
            const std::size_t populationSize) noexcept
        {
            return
                populationSize / 2U +
                populationSize % 2U;
        }

        [[nodiscard]]
        std::size_t logicalToPhysicalIndex(
            const std::size_t logicalIndex,
            const std::size_t archiveSize,
            const BNTGPObjective objective) noexcept
        {
            assert(logicalIndex < archiveSize);

            if (objective == BNTGPObjective::Makespan)
            {
                return logicalIndex;
            }

            return archiveSize - 1U - logicalIndex;
        }

        [[nodiscard]]
        const BNTGPEvaluation& evaluationAtLogicalIndex(
            const BNTGPArchive& archive,
            const std::size_t logicalIndex,
            const BNTGPObjective objective) noexcept
        {
            const std::size_t physicalIndex =
                logicalToPhysicalIndex(
                    logicalIndex,
                    archive.size(),
                    objective
                );

            return archive
                .entryAt(physicalIndex)
                .individual()
                .evaluation();
        }
    }

    BNTGPGapSelection::BNTGPGapSelection(
        const int tournamentSize) noexcept
        : tournamentSize_(tournamentSize)
    {
        assert(tournamentSize_ > 0);
    }

    std::size_t
        BNTGPGapSelection::selectParentByTournament(
            std::mt19937& randomEngine) const
    {
        assert(!gapValues_.empty());

        assert(
            gapValues_.size() <=
            static_cast<std::size_t>(
                std::numeric_limits<int>::max()
            )
        );

        const int maximumIndex =
            static_cast<int>(gapValues_.size()) - 1;

        int parentIndex = randomInt(
            randomEngine,
            0,
            maximumIndex
        );

        double bestGap =
            gapValues_[
                static_cast<std::size_t>(parentIndex)
            ];

        for (int tournamentIndex = 1;
            tournamentIndex < tournamentSize_;
            ++tournamentIndex)
        {
            const int randomIndex = randomInt(
                randomEngine,
                0,
                maximumIndex
            );

            const double candidateGap =
                gapValues_[
                    static_cast<std::size_t>(randomIndex)
                ];

            if (candidateGap > bestGap)
            {
                bestGap = candidateGap;
                parentIndex = randomIndex;
            }
        }

        return static_cast<std::size_t>(parentIndex);
    }

    const std::vector<BNTGPParentPair>&
        BNTGPGapSelection::select(
            BNTGPArchive& archive,
            const std::size_t populationSize,
            std::mt19937& randomEngine)
    {
        selectedParents_.clear();

        if (archive.size() < 2U)
        {
            if (!archive.empty())
            {
                selectedParents_.push_back({
                    0U,
                    0U,
                    BNTGPObjective::Makespan,
                    0.0,
                    0.0
                });
            }

            return selectedParents_;
        }

        const int objectiveIndex = randomInt(
            randomEngine,
            0,
            1
        );

        const BNTGPObjective objective =
            objectiveIndex == 0
            ? BNTGPObjective::Makespan
            : BNTGPObjective::Cost;

        const std::size_t archiveSize =
            archive.size();

        gapValues_.assign(
            archiveSize,
            0.0
        );

        gapValues_.front() =
            std::numeric_limits<double>::max();

        gapValues_.back() =
            std::numeric_limits<double>::max();

        for (std::size_t logicalIndex = 1U;
            logicalIndex + 1U < archiveSize;
            ++logicalIndex)
        {
            const double currentValue =
                normalizedObjectiveValue(
                    evaluationAtLogicalIndex(
                        archive,
                        logicalIndex,
                        objective
                    ),
                    objective
                );

            const double previousValue =
                normalizedObjectiveValue(
                    evaluationAtLogicalIndex(
                        archive,
                        logicalIndex - 1U,
                        objective
                    ),
                    objective
                );

            const double nextValue =
                normalizedObjectiveValue(
                    evaluationAtLogicalIndex(
                        archive,
                        logicalIndex + 1U,
                        objective
                    ),
                    objective
                );

            gapValues_[logicalIndex] = std::max(
                currentValue - previousValue,
                nextValue - currentValue
            );
        }

        for (std::size_t logicalIndex = 0U;
            logicalIndex < archiveSize;
            ++logicalIndex)
        {
            const std::size_t physicalIndex =
                logicalToPhysicalIndex(
                    logicalIndex,
                    archiveSize,
                    objective
                );

            const std::size_t selectionCount =
                archive
                .entryAt(physicalIndex)
                .selectionCount();

            gapValues_[logicalIndex] /=
                static_cast<double>(
                    selectionCount + 1U
                );
        }

        const std::size_t pairCount =
            requiredParentPairCount(populationSize);

        selectedParents_.reserve(pairCount);

        for (std::size_t offspringIndex = 0U;
            offspringIndex < populationSize;
            offspringIndex += 2U)
        {
            const std::size_t firstLogicalIndex =
                selectParentByTournament(randomEngine);

            std::size_t secondLogicalIndex = 0U;

            if (firstLogicalIndex == 0U)
            {
                secondLogicalIndex = 1U;
            }
            else if (
                firstLogicalIndex ==
                archiveSize - 1U)
            {
                secondLogicalIndex =
                    archiveSize - 2U;
            }
            else
            {
                const int neighborDirection = randomInt(
                    randomEngine,
                    0,
                    1
                );

                secondLogicalIndex =
                    neighborDirection == 0
                    ? firstLogicalIndex + 1U
                    : firstLogicalIndex - 1U;
            }

            const std::size_t firstPhysicalIndex =
                logicalToPhysicalIndex(
                    firstLogicalIndex,
                    archiveSize,
                    objective
                );

            const std::size_t secondPhysicalIndex =
                logicalToPhysicalIndex(
                    secondLogicalIndex,
                    archiveSize,
                    objective
                );

            archive
                .entryAt(firstPhysicalIndex)
                .recordSelection();

            archive
                .entryAt(secondPhysicalIndex)
                .recordSelection();

            selectedParents_.push_back({
                firstPhysicalIndex,
                secondPhysicalIndex,
                objective,
                gapValues_[firstLogicalIndex],
                gapValues_[secondLogicalIndex]
            });
        }

        return selectedParents_;
    }
}
