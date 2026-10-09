#pragma once

#include "../../archive/BNTGPArchive.hpp"
#include "../../archive/BNTGPObjectiveComparison.hpp"

#include <cstddef>
#include <random>
#include <vector>

namespace bntgp
{
    struct BNTGPParentPair final
    {
        std::size_t firstIndex{ 0U };
        std::size_t secondIndex{ 0U };

        BNTGPObjective objective{
            BNTGPObjective::Makespan
        };

        double firstGap{ 0.0 };
        double secondGap{ 0.0 };
    };

    class BNTGPGapSelection final
    {
    public:
        explicit BNTGPGapSelection(
            int tournamentSize
        ) noexcept;

        [[nodiscard]]
        const std::vector<BNTGPParentPair>& select(
            BNTGPArchive& archive,
            std::size_t populationSize,
            std::mt19937& randomEngine
        );

    private:
        [[nodiscard]]
        std::size_t selectParentByTournament(
            std::mt19937& randomEngine
        ) const;

        int tournamentSize_;

        std::vector<double> gapValues_{};
        std::vector<BNTGPParentPair> selectedParents_{};
    };
}
