#include "BNTGPPopulationInitializer.hpp"

#include "../../evaluation/BNTGPIndividualEvaluator.hpp"
#include "../../gp/GPTreeGenerator.hpp"

#include <cassert>
#include <cstddef>
#include <limits>
#include <utility>

namespace bntgp
{
    namespace
    {
        [[nodiscard]]
        BNTGPIndividualId takeNextIndividualId(
            BNTGPIndividualId& nextIndividualId) noexcept
        {
            assert(
                nextIndividualId !=
                std::numeric_limits<BNTGPIndividualId>::max()
            );

            const BNTGPIndividualId assignedId =
                nextIndividualId;

            ++nextIndividualId;

            return assignedId;
        }
    }

    void initializeBNTGPPopulation(
        std::vector<BNTGPIndividual>& population,
        const std::size_t populationSize,
        const int maximumTreeDepth,
        std::mt19937& randomEngine,
        BNTGPIndividualEvaluator& evaluator,
        BNTGPIndividualId& nextIndividualId)
    {
        assert(maximumTreeDepth >= 0);

        population.clear();
        population.reserve(populationSize);

        while (population.size() < populationSize)
        {
            gp::GPTree tree =
                gp::generateRandomPairTree(
                    randomEngine,
                    maximumTreeDepth
                );

            BNTGPIndividual individual{
                std::move(tree)
            };

            individual.setId(
                takeNextIndividualId(nextIndividualId)
            );

            evaluator.evaluate(individual);

            population.push_back(
                std::move(individual)
            );
        }
    }
}
