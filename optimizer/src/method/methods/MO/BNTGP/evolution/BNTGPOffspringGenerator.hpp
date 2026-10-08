#pragma once

#include "../core/BNTGPIndividual.hpp"

#include <cstddef>
#include <random>
#include <vector>

namespace bntgp
{
    class BNTGPArchive;
    class BNTGPGapSelection;
    class BNTGPVariation;
    class BNTGPIndividualEvaluator;
    struct BNTGPGenerationTrace;

    void generateBNTGPOffspring(
        std::vector<BNTGPIndividual>& offspring,
        BNTGPArchive& archive,
        std::size_t targetPopulationSize,
        BNTGPGapSelection& parentSelection,
        BNTGPVariation& variation,
        BNTGPIndividualEvaluator& evaluator,
        std::mt19937& randomEngine,
        BNTGPIndividualId& nextIndividualId,
        std::size_t generation,
        BNTGPGenerationTrace* generationTrace = nullptr,
        bool captureTreeSnapshots = false
    );
}
