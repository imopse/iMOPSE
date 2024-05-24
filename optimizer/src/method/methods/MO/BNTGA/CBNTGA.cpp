#include <algorithm>
#include <sstream>
#include "CBNTGA.h"
#include "../utils/archive/ArchiveUtils.h"
#include "../../../../utils/logger/ErrorUtils.h"
#include <utils/logger/CExperimentLogger.h>

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

        for (SMOIndividual* ind : m_Population)
        {
            delete ind;
        }
        m_Population = m_NextPopulation;
        m_NextPopulation.clear();
        m_NextPopulation.reserve(m_Population.size());

        generation++;
    }
    
    CExperimentLogger::LogProgress(1);
    ArchiveUtils::LogParetoFront(m_Archive);
    for (int i = 0; i < m_Archive.size(); i++) {
        m_Problem.LogSolution(*m_Archive[i]);
    }
    CExperimentLogger::LogData();
    m_Problem.LogAdditionalData();
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
