#include <algorithm>
#include <sstream>
#include "CSPEA2.h"
#include "../utils/archive/ArchiveUtils.h"
#include "../utils/DasDennis/CDasDennis.h"
#include "../../../../utils/random/CRandom.h"
#include "../../../../utils/logger/ErrorUtils.h"

CSPEA2::CSPEA2(AProblem &problem, AInitialization &initialization, ACrossover &crossover, AMutation &mutation, SConfigMap *configMap)
        : AMOGeneticMethod(problem, initialization, crossover, mutation)
{
    configMap->TakeValue("GenerationLimit", m_GenerationLimit);
    ErrorUtils::LowerThanZeroI("CSPEA2", "GenerationLimit", m_GenerationLimit);

    configMap->TakeValue("PopulationSize", m_PopulationSize);
    ErrorUtils::LowerThanZeroI("CSPEA2", "PopulationSize", m_PopulationSize);
    m_Population.reserve(m_PopulationSize);

    configMap->TakeValue("ArchiveSize", m_ArchiveSize);
    ErrorUtils::LowerThanZeroI("CSPEA2", "ArchiveSize", m_ArchiveSize);
    m_Archive.reserve(m_ArchiveSize);
}


void CSPEA2::RunOptimization()
{
    int generation = 0;

    for (size_t i = 0; i < m_PopulationSize; ++i)
    {
        SProblemEncoding& problemEncoding = m_Problem.GetProblemEncoding();
        auto* newInd = m_Initialization.CreateMOIndividual(problemEncoding);

        m_Problem.Evaluate(*newInd);

        m_Population.push_back(newInd);
    }

    std::vector<TNeighborhood> neighborhood;
    BuildNeighborhood(m_Population, neighborhood);
    UpdateFineGrainedFitness(m_Population, neighborhood);
    
    ArchiveUtils::CopyToArchiveWithFiltering(m_Population, m_Archive);

    while ( generation < m_GenerationLimit)
    {
        EvolveToNextGeneration();

        std::vector<SMOIndividual*> combinedPop;
        combinedPop.reserve(m_Population.size() + m_Archive.size());
        combinedPop.insert(combinedPop.end(), m_Population.begin(), m_Population.end());
        combinedPop.insert(combinedPop.end(), m_Archive.begin(), m_Archive.end());

        BuildNeighborhood(combinedPop, neighborhood);
        UpdateFineGrainedFitness(combinedPop, neighborhood);
        EnviroSelection(combinedPop);
        
        generation++;
    }

    std::vector<SMOIndividual*> allArchiveInd = m_Archive;
    m_Archive.clear();
    ArchiveUtils::CopyToArchiveWithFiltering(allArchiveInd, m_Archive);
    for (SMOIndividual* ind : allArchiveInd)
    {
        delete ind;
    }
    
    ArchiveUtils::CopyToArchiveWithFiltering(m_NextPopulation, m_Archive);
    ArchiveUtils::LogParetoFront(m_Archive);
}

void CSPEA2::EvolveToNextGeneration()
{
    for (size_t i = 0; i < m_Population.size(); i += 2)
    {
        size_t firstParentIdx = Spea2TournamentSelection(m_Archive);
        size_t secondParentIdx = Spea2TournamentSelection(m_Archive);
        SMOIndividual* firstParent = m_Archive[firstParentIdx];
        SMOIndividual* secondParent = m_Archive[secondParentIdx];

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

        delete m_Population[i];
        delete m_Population[i + 1];
        m_Population[i] = firstChild;
        m_Population[i + 1] = secondChild;
    }
}

void CSPEA2::BuildNeighborhood(std::vector<SMOIndividual *>& individuals, std::vector<TNeighborhood>& neighborhood)
{
    neighborhood = std::vector<TNeighborhood>(individuals.size(), TNeighborhood(individuals.size() - 1));
    size_t popSize = individuals.size();

    for (size_t i = 0; i < popSize; ++i)
    {
        size_t entryIdx = 0;
        for (size_t j = 0; j < popSize; ++j)
        {
            // just skip if self
            if (i == j)
            {
                continue;
            }

            std::pair<size_t, float>& entry = neighborhood[i][entryIdx];
            entry.first = j;
            entry.second = CalcDist(*individuals[i], *individuals[j]);
            ++entryIdx;
        }
    }

    // Sort ascending by distance
    for (TNeighborhood& nh : neighborhood)
    {
        std::sort(nh.begin(), nh.end(), [](const std::pair<size_t, float>& a, const std::pair<size_t, float>& b) -> bool
        {
            return a.second < b.second;
        });
    }
}

float CSPEA2::CalcDist(const SMOIndividual& leftInd, const SMOIndividual& rightInd)
{
    size_t objCount = m_Problem.GetProblemEncoding().m_objectivesNumber;
    float dist = 0.f;
    for (int i = 0; i < objCount; ++i)
    {
        // non norm
        dist += powf(rightInd.m_NormalizedEvaluation[i] - leftInd.m_NormalizedEvaluation[i], 2);
    }
    return sqrtf(dist);
}

void CSPEA2::UpdateFineGrainedFitness(std::vector<SMOIndividual*>& individuals, const std::vector<TNeighborhood>& neighborhood)
{
    UpdateRawFitness(individuals);
    UpdateDensity(individuals, neighborhood);
}

void CSPEA2::UpdateRawFitness(std::vector<SMOIndividual*>& individuals)
{
    size_t indSize = individuals.size();
    std::vector<size_t> strengthVector(indSize, 0);
    std::vector<std::vector<size_t>> dominators(indSize, std::vector<size_t>());

    // R(i) -> rank
    for (size_t i = 0; i < indSize; ++i)
    {
        for (size_t j = 0; j < indSize; ++j)
        {
            if (i == j)
                continue;

            if (individuals[i]->IsDominatedBy(individuals[j]))
            {
                // Mark that Ind J dominates Ind I
                dominators[i].push_back(j);
                // Increase Ind J strength
                strengthVector[j] += 1;
            }
        }
    }

    for (size_t i = 0; i < indSize; ++i)
    {
        size_t totalDominatorsStrength = 0;
        for (size_t domIdx : dominators[i])
        {
            totalDominatorsStrength += strengthVector[domIdx];
        }
        individuals[i]->m_Rank = totalDominatorsStrength;
    }
}

void CSPEA2::UpdateDensity(std::vector<SMOIndividual*>& individuals, const std::vector<TNeighborhood>& neighborhood)
{
    // D(i) -> CrowdDist
    size_t k = sqrtf(m_PopulationSize + m_ArchiveSize);

    for (size_t i = 0; i < individuals.size(); ++i)
    {
        individuals[i]->m_CrowdingDistance = neighborhood[i][k].second;
    }
}

void CSPEA2::EnviroSelection(std::vector<SMOIndividual*>& individuals)
{
    auto* archiveCopy = new std::vector<SMOIndividual*>();
    archiveCopy->reserve(m_Archive.size());  // Reserve space
    std::copy(m_Archive.begin(), m_Archive.end(), std::back_inserter(*archiveCopy));  // Copy pointers

    m_Archive.clear();
    
    std::vector<const SMOIndividual*> nonDominatedIndividuals;
    std::vector<const SMOIndividual*> dominatedIndividuals;
    SplitByDomination(individuals, dominatedIndividuals, nonDominatedIndividuals);

    int sizeDiff = (int)m_ArchiveSize - (int)nonDominatedIndividuals.size();
    if (sizeDiff > 0)
    {
        // Add dominated individuals with good results
        std::sort(dominatedIndividuals.begin(), dominatedIndividuals.end(), [](const SMOIndividual* a, const SMOIndividual* b) -> bool
        {
            return (a->m_Rank + a->m_CrowdingDistance) < (b->m_Rank + b->m_CrowdingDistance);
        });
        for (int i = 0; i < sizeDiff; ++i)
        {
            nonDominatedIndividuals.push_back(dominatedIndividuals[i]);
        }
    }
    else if (nonDominatedIndividuals.size() > m_ArchiveSize)
    {
        // Remove most crowded individuals
        TruncateByDistance(nonDominatedIndividuals, m_ArchiveSize);
    }
    
    // Finally, add new individuals to the archive
    for (const SMOIndividual* filteredInd : nonDominatedIndividuals)
    {
        m_Archive.push_back(new SMOIndividual(*filteredInd));
    }
    
    // Ensure that old archive is deleted
    for (auto* oldIndividual: *archiveCopy) {
        delete oldIndividual;
    }
    delete archiveCopy;
}

void CSPEA2::SplitByDomination(std::vector<SMOIndividual*>& individuals, std::vector<const SMOIndividual*>& dominatedIndividuals, std::vector<const SMOIndividual*>& nonDominatedIndividuals)
{
    nonDominatedIndividuals.clear();
    nonDominatedIndividuals.reserve(individuals.size());
    dominatedIndividuals.clear();
    dominatedIndividuals.reserve(individuals.size());
    // For each new individual, check if not dominated
    // We don't have to check if the same solution, equal rewards are discarded
    for (size_t p = 0; p < individuals.size(); ++p)
    {
        const SMOIndividual* newInd = individuals[p];
        bool isDominated = false;
        // Check other new individuals
        size_t i = 0;
        while (!isDominated && i < individuals.size())
        {
            if (p != i)
            {
                isDominated = newInd->IsDominatedBy(individuals[i]);
                if (!isDominated && p < i) // If DuplicateFenotype - leave the last one
                {
                    // Check if duplicated
                    isDominated = newInd->IsDuplicateEvalValue(individuals[i]);
                }
            }
            ++i;
        }
        if (isDominated)
        {
            dominatedIndividuals.push_back(newInd);
        }
        else
        {
            nonDominatedIndividuals.push_back(newInd);
        }
    }
}

void CSPEA2::TruncateByDistance(std::vector<const SMOIndividual*>& filteredIndividuals, size_t maxSize)
{
    // also simplified - remove first closest point
    while (filteredIndividuals.size() > maxSize)
    {
        float maxDist = 0.f;
        size_t toRemove = 0;
        for (size_t i = 0; i < filteredIndividuals.size(); ++i)
        {
            for (size_t j = i + 1; j < filteredIndividuals.size(); ++j)
            {
                float d = CalcDist(*filteredIndividuals[i], *filteredIndividuals[j]);
                if (d > maxDist)
                {
                    maxDist = d;
                    toRemove = i;
                }
            }
        }

        filteredIndividuals.erase(filteredIndividuals.begin() + toRemove);
    }
}

size_t CSPEA2::Spea2TournamentSelection(const std::vector<SMOIndividual*>& population)
{
    // binary selection
    size_t tournamentSize = 2;
    size_t popSize = population.size();

    size_t bestIdx = CRandom::GetInt(0, popSize);
    // fitness to be minimized
    float bestFitness = (float)population[bestIdx]->m_Rank + population[bestIdx]->m_CrowdingDistance;

    for (size_t i = 1; i < tournamentSize; ++i)
    {
        size_t randomIdx = CRandom::GetInt(0, popSize);
        float fitness = (float)population[randomIdx]->m_Rank + population[randomIdx]->m_CrowdingDistance;
        if (fitness < bestFitness)
        {
            bestFitness = fitness;
            bestIdx = randomIdx;
        }
    }

    return bestIdx;
}