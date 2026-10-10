#include "BNTGPConfigReader.hpp"

#include "method/configMap/SConfigMap.h"

#include <cmath>
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace bntgp
{
    namespace
    {
        [[nodiscard]]
        int ReadInt(
            SConfigMap& config,
            const char* key,
            const int defaultValue)
        {
            int value = defaultValue;
            config.TakeValue(key, value);
            return value;
        }

        [[nodiscard]]
        double ReadDouble(
            SConfigMap& config,
            const char* key,
            const double defaultValue)
        {
            double value = defaultValue;
            config.TakeValue(key, value);
            return value;
        }

        [[nodiscard]]
        bool ReadBoolean(
            SConfigMap& config,
            const char* key,
            const bool defaultValue)
        {
            const int numericDefault = defaultValue ? 1 : 0;
            const int value = ReadInt(config, key, numericDefault);

            return value != 0;
        }

        void ValidatePopulationSize(const int value)
        {
            if (value <= 0)
            {
                throw std::invalid_argument(
                    "BNTGP configuration: PopulationSize must be greater "
                    "than zero.");
            }
        }

        void ValidateGenerationCount(const int value)
        {
            if (value < 0)
            {
                throw std::invalid_argument(
                    "BNTGP configuration: Generations cannot be negative.");
            }
        }

        void ValidateTournamentSize(const int value)
        {
            if (value <= 0)
            {
                throw std::invalid_argument(
                    "BNTGP configuration: TournamentSize must be greater "
                    "than zero.");
            }
        }

        void ValidateMaximumTreeDepth(const int value)
        {
            if (value <= 0)
            {
                throw std::invalid_argument(
                    "BNTGP configuration: MaxDepth must be greater "
                    "than zero.");
            }
        }

        void ValidateSamplingInterval(const int value)
        {
            if (value < 0)
            {
                throw std::invalid_argument(
                    "BNTGP configuration: NodeStatsEvery cannot be negative.");
            }
        }

        void ValidatePositiveSamplingInterval(
            const int value,
            const char* parameterName)
        {
            if (value <= 0)
            {
                throw std::invalid_argument(
                    std::string("BNTGP configuration: ") +
                    parameterName +
                    " must be greater than zero.");
            }
        }

        void ValidateBufferSizeKilobytes(const int value)
        {
            if (value <= 0)
            {
                throw std::invalid_argument(
                    "BNTGP configuration: ParetoLineageBufferKB must be "
                    "greater than zero.");
            }
        }

        void ValidateProbability(
            const double value,
            const char* parameterName)
        {
            if (!std::isfinite(value) || value < 0.0 || value > 1.0)
            {
                throw std::invalid_argument(
                    std::string("BNTGP configuration: ") +
                    parameterName +
                    " must be a finite value in the range [0, 1].");
            }
        }
    }

    BNTGPParameters ReadBNTGPConfiguration(
        const SConfigMap& sourceConfig)
    {
        SConfigMap config = sourceConfig;

        BNTGPParameters parameters{};

        const int populationSize = ReadInt(
            config,
            "PopulationSize",
            static_cast<int>(parameters.evolution.populationSize));

        const int generationCount = ReadInt(
            config,
            "Generations",
            static_cast<int>(parameters.evolution.generationCount));

        const int tournamentSize = ReadInt(
            config,
            "TournamentSize",
            parameters.evolution.tournamentSize);

        ValidatePopulationSize(populationSize);
        ValidateGenerationCount(generationCount);
        ValidateTournamentSize(tournamentSize);

        parameters.evolution.populationSize =
            static_cast<std::size_t>(populationSize);

        parameters.evolution.generationCount =
            static_cast<std::size_t>(generationCount);

        parameters.evolution.tournamentSize = tournamentSize;

        double parameterMutationProbability = ReadDouble(
            config,
            "MutationProbParam",
            parameters.variation.parameterMutationProbability);

        double structuralMutationProbability = ReadDouble(
            config,
            "MutationProbStruct",
            parameters.variation.structuralMutationProbability);

        const double generalMutationFallback = ReadDouble(
            config,
            "MutationProb",
            parameterMutationProbability);

        parameterMutationProbability = ReadDouble(
            config,
            "ParamMutationProb",
            generalMutationFallback);

        structuralMutationProbability = ReadDouble(
            config,
            "StructMutationProb",
            structuralMutationProbability);

        const double crossoverProbability = ReadDouble(
            config,
            "CrossoverProb",
            parameters.variation.crossoverProbability);

        ValidateProbability(
            crossoverProbability,
            "CrossoverProb");

        ValidateProbability(
            parameterMutationProbability,
            "parameter mutation probability");

        ValidateProbability(
            structuralMutationProbability,
            "structural mutation probability");

        parameters.variation.crossoverProbability =
            crossoverProbability;

        parameters.variation.parameterMutationProbability =
            parameterMutationProbability;

        parameters.variation.structuralMutationProbability =
            structuralMutationProbability;

        const int maximumDepth = ReadInt(
            config,
            "MaxDepth",
            parameters.tree.maximumDepth);

        ValidateMaximumTreeDepth(maximumDepth);

        parameters.tree.maximumDepth = maximumDepth;

        const int initialDepth = ReadInt(config, "InitialTreeMaxDepth", parameters.tree.maximumDepth);
        ValidateMaximumTreeDepth(initialDepth);
        parameters.tree.maximumDepth = initialDepth;
        parameters.tree.softDepthEnabled = ReadBoolean(config, "SoftDepthEnabled", parameters.tree.softDepthEnabled);
        parameters.tree.softDepthFreeEdges = ReadInt(config, "SoftDepthFreeEdges", parameters.tree.softDepthFreeEdges);
        parameters.tree.softDepthMakespanPenalty = ReadDouble(config, "SoftDepthMakespanPenalty", parameters.tree.softDepthMakespanPenalty);
        parameters.tree.softDepthCostPenalty = ReadDouble(config, "SoftDepthCostPenalty", parameters.tree.softDepthCostPenalty);
        if (parameters.tree.softDepthFreeEdges < 0 || !std::isfinite(parameters.tree.softDepthMakespanPenalty)
                || !std::isfinite(parameters.tree.softDepthCostPenalty)
                || parameters.tree.softDepthMakespanPenalty < 0.0 || parameters.tree.softDepthCostPenalty < 0.0)
            throw std::invalid_argument("BNTGP: invalid SOFT depth penalty configuration");

        std::string featureSet = "R23";
        config.TakeValue("FeatureSet", featureSet);
        std::transform(featureSet.begin(), featureSet.end(), featureSet.begin(),
            [](unsigned char ch){return static_cast<char>(std::toupper(ch));});
        if (featureSet == "R23") parameters.enabledFeatures = gp::r23GPFeatures();
        else if (featureSet == "FULL30" || featureSet == "ALL" || featureSet == "R30")
            parameters.enabledFeatures = gp::allGPFeatures();
        else throw std::invalid_argument("BNTGP: FeatureSet must be R23 or FULL30");
        std::string enabledFeatures;
        if (config.TakeValue("EnabledFeatures", enabledFeatures)) {
            parameters.enabledFeatures.fill(false);
            gp::applyFeatureList(parameters.enabledFeatures, enabledFeatures, true);
        }
        std::string disabledFeatures;
        if (config.TakeValue("DisabledFeatures", disabledFeatures))
            gp::applyFeatureList(parameters.enabledFeatures, disabledFeatures, false);

        parameters.logging.enabled = ReadBoolean(
            config,
            "EnableLoggers",
            parameters.logging.enabled);

        parameters.logging.nodeStatistics.enabled = ReadBoolean(
            config,
            "LogNodeDistribution",
            parameters.logging.nodeStatistics.enabled);

        const int samplingInterval = ReadInt(
            config,
            "NodeStatsEvery",
            static_cast<int>(
                parameters.logging.nodeStatistics.samplingInterval));

        ValidateSamplingInterval(samplingInterval);

        parameters.logging.nodeStatistics.samplingInterval =
            static_cast<std::size_t>(samplingInterval);

        parameters.logging.nodeStatistics.useArchive = ReadBoolean(
            config,
            "NodeStatsUseArchive",
            parameters.logging.nodeStatistics.useArchive);

        parameters.logging.paretoLineage.enabled = ReadBoolean(
            config,
            "LogParetoLineage",
            parameters.logging.paretoLineage.enabled);

        const int paretoLineageSamplingInterval = ReadInt(
            config,
            "ParetoLineageEvery",
            static_cast<int>(
                parameters.logging.paretoLineage.samplingInterval));

        ValidatePositiveSamplingInterval(
            paretoLineageSamplingInterval,
            "ParetoLineageEvery");

        parameters.logging.paretoLineage.samplingInterval =
            static_cast<std::size_t>(
                paretoLineageSamplingInterval);

        parameters.logging.paretoLineage.acceptedOnly = ReadBoolean(
            config,
            "ParetoLineageAcceptedOnly",
            parameters.logging.paretoLineage.acceptedOnly);

        parameters.logging.paretoLineage.logArchiveDeltas = ReadBoolean(
            config,
            "ParetoArchiveDeltas",
            parameters.logging.paretoLineage.logArchiveDeltas);

        const int paretoLineageBufferKilobytes = ReadInt(
            config,
            "ParetoLineageBufferKB",
            static_cast<int>(
                parameters.logging.paretoLineage
                .streamBufferSizeBytes / 1024U));

        ValidateBufferSizeKilobytes(
            paretoLineageBufferKilobytes);

        parameters.logging.paretoLineage.streamBufferSizeBytes =
            static_cast<std::size_t>(
                paretoLineageBufferKilobytes) * 1024U;

        parameters.logging.objectiveStn.enabled = ReadBoolean(
            config,
            "LogObjectiveSTN",
            parameters.logging.objectiveStn.enabled);

        const int objectiveStnBufferKilobytes = ReadInt(
            config,
            "ObjectiveSTNBufferKB",
            static_cast<int>(
                parameters.logging.objectiveStn
                .streamBufferSizeBytes / 1024U));

        ValidatePositiveSamplingInterval(
            objectiveStnBufferKilobytes,
            "ObjectiveSTNBufferKB");

        parameters.logging.objectiveStn.streamBufferSizeBytes =
            static_cast<std::size_t>(
                objectiveStnBufferKilobytes) * 1024U;

        parameters.logging.fullTreeTrace.enabled = ReadBoolean(
            config,
            "LogFullTreeTrace",
            parameters.logging.fullTreeTrace.enabled);

        const int fullTreeTraceBufferKilobytes = ReadInt(
            config,
            "FullTreeTraceBufferKB",
            static_cast<int>(
                parameters.logging.fullTreeTrace
                .streamBufferSizeBytes / 1024U));

        ValidatePositiveSamplingInterval(
            fullTreeTraceBufferKilobytes,
            "FullTreeTraceBufferKB");

        parameters.logging.fullTreeTrace.streamBufferSizeBytes =
            static_cast<std::size_t>(
                fullTreeTraceBufferKilobytes) * 1024U;

        int configuredSeed = 0;

        if (config.TakeValue("Seed", configuredSeed))
        {
            parameters.seedOverride =
                static_cast<std::uint64_t>(configuredSeed);
        }

        return parameters;
    }
}