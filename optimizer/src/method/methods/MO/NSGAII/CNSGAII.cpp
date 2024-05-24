#include <cfloat>
#include <algorithm>
#include <sstream>
#include "CNSGAII.h"
#include "../utils/archive/ArchiveUtils.h"
#include "../utils/clustering/CNonDominatedSorting.h"
#include "../../../../utils/logger/ErrorUtils.h"
#include <utils/logger/CExperimentLogger.h>

CNSGAII::CNSGAII(AProblem &evaluator,
                 AInitialization &initialization,
                 CRankedTournament &rankedTournament,
                 ACrossover &crossover,
                 AMutation &mutation,
                 SConfigMap *configMap
) :
        AMOGeneticMethod(evaluator, initialization, crossover, mutation),
        m_RankedTournament(rankedTournament)
{
    configMap->TakeValue("PopulationSize", m_PopulationSize);
    ErrorUtils::LowerThanZeroI("NSGAII", "PopulationSize", m_PopulationSize);
    m_Population.reserve(m_PopulationSize);
    m_NextPopulation.reserve(m_PopulationSize);

    configMap->TakeValue("GenerationLimit", m_GenerationLimit);
    ErrorUtils::LowerThanZeroI("NSGAII", "GenerationLimit", m_GenerationLimit);
}


void CNSGAII::RunOptimization()
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

        std::vector<SMOIndividual *> combinedPop;
        combinedPop.reserve(m_Population.size() + m_NextPopulation.size());
        combinedPop.insert(combinedPop.end(), m_Population.begin(), m_Population.end());
        combinedPop.insert(combinedPop.end(), m_NextPopulation.begin(), m_NextPopulation.end());

        CNonDominatedSorting nonDominatedSorting;
        std::vector<std::vector<size_t>> combinedClusters;
        nonDominatedSorting.Cluster(combinedPop, combinedClusters);

        std::vector<SMOIndividual *> tempPopulation;
        tempPopulation.reserve(m_PopulationSize);
        for (size_t i = 0; tempPopulation.size() < m_PopulationSize; ++i)
        {
            std::vector<size_t> &cluster = combinedClusters[i];
            // if this cluster exceeds the size
            if (tempPopulation.size() + cluster.size() > m_PopulationSize)
            {
                CalcCrowdingDistance(combinedPop, cluster);
                std::sort(cluster.begin(), cluster.end(), [combinedPop](const size_t &a, const size_t &b) -> bool
                {
                    return combinedPop[a]->m_CrowdingDistance < combinedPop[b]->m_CrowdingDistance;
                });
                for (size_t idx: cluster)
                {
                    // deep copy individuals as they fill be the new population
                    tempPopulation.push_back(new SMOIndividual(*combinedPop[idx]));
                    if (tempPopulation.size() >= m_PopulationSize)
                        break;
                }
            }
            else
            {
                for (size_t idx: cluster)
                {
                    tempPopulation.push_back(new SMOIndividual(*combinedPop[idx]));
                }
            }
        }

        // Clear both current and next population
        for (SMOIndividual *ind: combinedPop)
        {
            delete ind;
        }
        m_Population.clear();
        m_NextPopulation.clear();

        m_Population.swap(tempPopulation);
        m_NextPopulation.reserve(m_Population.size());

        generation++;
    }

    ArchiveUtils::CopyToArchiveWithFiltering(m_NextPopulation, m_Archive);

    CExperimentLogger::LogProgress(1);
    ArchiveUtils::LogParetoFront(m_Archive);
    for (int i = 0; i < m_Archive.size(); i++) {
        m_Problem.LogSolution(*m_Archive[i]);
    }
    CExperimentLogger::LogData();
    m_Problem.LogAdditionalData();
}

void CNSGAII::EvolveToNextGeneration()
{

    for (size_t i = 0; i < m_PopulationSize; i += 2)
    {
        auto *firstParent = m_RankedTournament.Select(m_Population);
        auto *secondParent = m_RankedTournament.Select(m_Population);

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
    ArchiveUtils::CopyToArchiveWithFiltering(m_NextPopulation, m_Archive);
}

void CNSGAII::CalcCrowdingDistance(std::vector<SMOIndividual *> &population, std::vector<size_t> &indices)
{
    for (int objIdx = 0; objIdx < m_Problem.GetProblemEncoding().m_objectivesNumber; ++objIdx)
    {
        std::sort(indices.begin(), indices.end(), [objIdx, population](const size_t &a, const size_t &b) -> bool
        {
            return population[a]->m_NormalizedEvaluation[objIdx] < population[b]->m_NormalizedEvaluation[objIdx];
        });

        population[indices[0]]->m_CrowdingDistance = FLT_MAX;
        population[indices[indices.size() - 1]]->m_CrowdingDistance = FLT_MAX;

        for (size_t i = 1; i < indices.size() - 1; ++i)
        {
            float currDist = population[indices[i]]->m_CrowdingDistance;
            float plusValue = population[indices[i + 1]]->m_NormalizedEvaluation[objIdx];
            float minusValue = population[indices[i - 1]]->m_NormalizedEvaluation[objIdx];
            population[indices[i]]->m_CrowdingDistance = currDist + plusValue - minusValue;
        }
    }
}