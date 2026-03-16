#include <algorithm>
#include <sstream>
#include "CBNTGA.h"
#include "../utils/archive/ArchiveUtils.h"
#include "../../../../utils/logger/ErrorUtils.h"
#include "utils/dataStructures/CCSV.h"
#include "utils/logger/CExperimentLogger.h"

CBNTGA::CBNTGA(AProblem &evaluator, AInitialization &initialization,
               ACrossover &crossover, AMutation &mutation, CGapSelectionByRandomDim& gapSelection, SConfigMap *configMap) :
        AMOGeneticMethod(evaluator, initialization, crossover, mutation), m_GapSelection(gapSelection)
{
    configMap->TakeValue("PopulationSize", m_PopulationSize);
    ErrorUtils::LowerThanZeroI("BNTGA", "PopulationSize", m_PopulationSize);
    m_Population.reserve(m_PopulationSize);
    m_NextPopulation.reserve(m_PopulationSize);

    configMap->TakeValue("GenerationLimit", m_GenerationLimit);
    ErrorUtils::LowerThanZeroI("BNTGA", "GenerationLimit", m_GenerationLimit);
}


void CBNTGA::RunOptimization()
{
    int generation = 0;

    m_StartTime = std::chrono::high_resolution_clock::now();
    CCSV<float> m_OperatorStats(1);
    CCSV<float> m_HVStats(4);

    for (size_t i = 0; i < m_PopulationSize; ++i)
    {
        SProblemEncoding& problemEncoding = m_Problem.GetProblemEncoding();
        auto* newInd = m_Initialization.CreateMOIndividual(problemEncoding);

        m_Problem.Evaluate(*newInd);

        m_Population.push_back(newInd);
    }

    ArchiveUtils::CopyToArchiveWithFiltering(m_Population, m_Archive);

    while (generation < m_GenerationLimit)
    {
        EvolveToNextGeneration();

        int fet = (generation + 1) * int(m_PopulationSize);

        // TODO - use functions
        {
            // It will sort the archive
            float paretoHV = CalcHV(m_Archive, std::vector<float>{ 1.f, 1.f });
            auto currentTime = std::chrono::high_resolution_clock::now();
            auto durationMS = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - m_StartTime);
            std::vector<float> newRow = { (float)generation, paretoHV, (float)durationMS.count(), float(fet) };
            m_HVStats.AddRow(std::move(newRow));
        }

        for (SMOIndividual* ind : m_Population)
        {
            delete ind;
        }
        m_Population = m_NextPopulation;
        m_NextPopulation.clear();
        m_NextPopulation.reserve(m_Population.size());

        generation++;

        if (fet % 5000 == 0)
        {
            ArchiveUtils::LogParetoFront(m_Archive, fet);
        }
    }
    
    ArchiveUtils::LogParetoFront(m_Archive);

    CExperimentLogger::LogResult(m_OperatorStats.ToStringStream().str().c_str(), "OperatorStats.csv");
    CExperimentLogger::LogResult(m_HVStats.ToStringStream().str().c_str(), "HVStats.csv");
}

void CBNTGA::EvolveToNextGeneration()
{
    const auto &parents = m_GapSelection.Select(
            m_Archive,
            m_Problem.GetProblemEncoding().m_objectivesNumber,
            m_PopulationSize
    );
    for (auto& parentPair : parents)
    {
        CrossoverAndMutate(parentPair.first, parentPair.second);
    }
    ArchiveUtils::CopyToArchiveWithFiltering(m_NextPopulation, m_Archive);
}

void CBNTGA::CrossoverAndMutate(SMOIndividual* firstParent, SMOIndividual* secondParent)
{
    auto *firstChild = new SMOIndividual{*firstParent};
    auto *secondChild = new SMOIndividual{*secondParent};

    m_Crossover.Crossover(
            m_Problem.GetProblemEncoding(),
            *firstParent,
            *secondParent,
            *firstChild,
            *secondChild
    );

    m_Mutation.Mutate(m_Problem.GetProblemEncoding(), *firstChild);
    m_Mutation.Mutate(m_Problem.GetProblemEncoding(), *secondChild);

    m_Problem.Evaluate(*firstChild);
    m_Problem.Evaluate(*secondChild);

    m_NextPopulation.emplace_back(firstChild);
    m_NextPopulation.emplace_back(secondChild);
}

// TODO - copied from ParetoAnalyzer - remove it or move somewhere else
// Calculate Hyper-volume using values as they are (either absolute or normalized), using the reference point
float CBNTGA::CalcHV(std::vector<SMOIndividual*>& individuals, const std::vector<float>& refPoint)
{
    auto sortLambda = [](const SMOIndividual* lhv, const SMOIndividual* rhv) -> bool
    {
        return lhv->m_NormalizedEvaluation[0] < rhv->m_NormalizedEvaluation[0];
    };

    std::sort(individuals.begin(), individuals.end(), sortLambda);

    float hyperVolume = 0.f;
    float prevCost = refPoint[1];
    for (const SMOIndividual* sol : individuals)
    {
        hyperVolume += ((refPoint[0] - sol->m_NormalizedEvaluation[0]) * (prevCost - sol->m_NormalizedEvaluation[1]));
        prevCost = sol->m_NormalizedEvaluation[1];
    }

    return hyperVolume;
}