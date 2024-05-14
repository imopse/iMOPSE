#include <algorithm>
#include <sstream>
#include "CNTGA2.h"
#include "../utils/archive/ArchiveUtils.h"
#include "../utils/clustering/CNonDominatedSorting.h"
#include "../../../../utils/logger/ErrorUtils.h"
#include "../../../../utils/logger/CExperimentLogger.h"

CNTGA2::CNTGA2(AProblem &evaluator,
               AInitialization &initialization,
               CRankedTournament &rankedTournament,
               CGapSelectionByRandomDim &gapSelection,
               ACrossover &crossover,
               AMutation &mutation,
               SConfigMap *configMap
) :
        AMOGeneticMethod(evaluator, initialization, crossover, mutation),
        m_RankedTournament(rankedTournament),
        m_GapSelection(gapSelection)
{
    configMap->TakeValue("GapSelectionPercent", m_GapSelectionPercent);
    ErrorUtils::OutOfScopeF("NTGA2", "GapSelectionPercent", m_GapSelectionPercent / 100.f);

    configMap->TakeValue("PopulationSize", m_PopulationSize);
    ErrorUtils::LowerThanZeroI("NTGA2", "PopulationSize", m_PopulationSize);
    m_Population.reserve(m_PopulationSize);
    m_NextPopulation.reserve(m_PopulationSize);

    configMap->TakeValue("GenerationLimit", m_GenerationLimit);
    ErrorUtils::LowerThanZeroI("NTGA2", "GenerationLimit", m_GenerationLimit);
}


void CNTGA2::RunOptimization()
{
    int generation = 0;

    for (size_t i = 0; i < m_PopulationSize; ++i)
    {
        SProblemEncoding& problemEncoding = m_Problem.GetProblemEncoding();
        auto* newInd = m_Initialization.CreateMOIndividualForECVRPTW(problemEncoding, (CECVRPTW&)m_Problem);

        m_Problem.Evaluate(*newInd);

        m_Population.push_back(newInd);
    }

    ArchiveUtils::CopyToArchiveWithFiltering(m_Population, m_Archive);

    while (generation < m_GenerationLimit)
    {
        CExperimentLogger::LogProgress(generation / (float)m_GenerationLimit);
        if ((generation % 100) < (100 - m_GapSelectionPercent))
        {
            // Without Gap
            std::vector<SMOIndividual *> parentsVector;
            parentsVector.reserve(m_Archive.size() + m_Population.size());
            parentsVector.insert(parentsVector.end(), m_Archive.begin(), m_Archive.end());
            parentsVector.insert(parentsVector.end(), m_Population.begin(), m_Population.end());

            CNonDominatedSorting nonDominatedSorting;
            std::vector<std::vector<size_t>> combinedClusters;
            nonDominatedSorting.Cluster(parentsVector, combinedClusters);

            for (size_t i = 0; i < m_PopulationSize; i += 2)
            {
                auto *firstParent = m_RankedTournament.Select(m_Population);
                auto *secondParent = m_RankedTournament.Select(m_Population);

                CrossoverAndMutate(*firstParent, *secondParent);
            }
        }
        else
        {
            // Evolve with Gap
            const auto &parents = m_GapSelection.Select(
                    m_Archive,
                    m_Problem.GetProblemEncoding().m_objectivesNumber,
                    m_PopulationSize
            );
            for (auto& parentPair : parents)
            {
                CrossoverAndMutate(*parentPair.first, *parentPair.second);
            }
        }
        ArchiveUtils::CopyToArchiveWithFiltering(m_NextPopulation, m_Archive);

        for (SMOIndividual *ind: m_Population)
        {
            delete ind;
        }
        m_Population = m_NextPopulation;
        m_NextPopulation.clear();
        m_NextPopulation.reserve(m_Population.size());

        ++generation;
    }

    CExperimentLogger::LogProgress(1);
    ArchiveUtils::LogParetoFront(m_Archive);
    for (int i = 0; i < m_Archive.size(); i++) {
        m_Problem.LogSolution(*m_Archive[i]);
    }
    CExperimentLogger::LogData();
    m_Problem.LogAdditionalData();
}

void CNTGA2::CrossoverAndMutate(SMOIndividual &firstParent, SMOIndividual &secondParent)
{
    auto *firstChild = new SMOIndividual{firstParent};
    auto *secondChild = new SMOIndividual{secondParent};

    m_Crossover.Crossover(
            m_Problem.GetProblemEncoding(),
            firstParent,
            secondParent,
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