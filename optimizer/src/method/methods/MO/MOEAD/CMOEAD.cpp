#include <algorithm>
#include <sstream>
#include "CMOEAD.h"
#include "../utils/archive/ArchiveUtils.h"
#include "../utils/DasDennis/CDasDennis.h"
#include "../../../../utils/random/CRandom.h"
#include "../../../../utils/logger/ErrorUtils.h"

CMOEAD::CMOEAD(AProblem &problem, AInitialization &initialization, ACrossover &crossover, AMutation &mutation, SConfigMap *configMap)
        : AMOGeneticMethod(problem, initialization, crossover, mutation)
{
    configMap->TakeValue("GenerationLimit", m_GenerationLimit);
    ErrorUtils::LowerThanZeroI("CMOEAD", "GenerationLimit", m_GenerationLimit);

    configMap->TakeValue("PartitionsNumber", m_PartitionsNumber);
    ErrorUtils::LowerThanZeroI("CMOEAD", "PartitionsNumber", m_PartitionsNumber);

    configMap->TakeValue("NeighbourhoodSize", m_NeighbourhoodSize);
    ErrorUtils::LowerThanZeroI("CMOEAD", "NeighbourhoodSize", m_NeighbourhoodSize);
}


void CMOEAD::RunOptimization()
{
    int generation = 0;
    
    ConstructSubproblems(m_PartitionsNumber, m_NeighbourhoodSize);
    m_PopulationSize = m_Subproblems.size();
    m_Population.reserve(m_PopulationSize);

    for (size_t i = 0; i < m_PopulationSize; ++i)
    {
        SProblemEncoding& problemEncoding = m_Problem.GetProblemEncoding();
        auto* newInd = m_Initialization.CreateMOIndividual(problemEncoding);

        m_Problem.Evaluate(*newInd);

        m_Population.push_back(newInd);
    }
    
    ArchiveUtils::CopyToArchiveWithFiltering(m_Population, m_Archive);

    while ( generation < m_GenerationLimit)
    {
        EvolveToNextGeneration();
        generation++;
    }

    ArchiveUtils::LogParetoFront(m_Archive);
}

void CMOEAD::ConstructSubproblems(size_t partitionsNumber, size_t neighborhoodSize)
{
    size_t dimCount = m_Problem.GetProblemEncoding().m_objectivesNumber;

    if (dimCount == 2)
    {
        ConstructSubproblemsSimple2D(partitionsNumber, neighborhoodSize);
    }
    else
    {
        ConstructSubproblemsMultiD(partitionsNumber, neighborhoodSize, dimCount);
    }
}

void CMOEAD::ConstructSubproblemsSimple2D(size_t partitionsNumber, size_t neighborhoodSize)
{
    m_Subproblems.clear();

    // Simple impl for 2dim problems only
    float stepVal = 1.f / (partitionsNumber - 1);
    for (size_t i = 0; i < partitionsNumber; ++i)
    {
        m_Subproblems.emplace_back();
        SSubproblem& sp = m_Subproblems.back();
        sp.m_WeightVector = { 0.f + i * stepVal, 1.f - i * stepVal};
    }

    // For 2 dim, we assume they are ordered
    for (size_t i = 0; i < partitionsNumber; ++i)
    {
        int leftIdx = (int)i - ((int)neighborhoodSize / 2);
        if (leftIdx < 0) leftIdx = 0;
        size_t rightIdx = leftIdx + neighborhoodSize;
        if (rightIdx >= partitionsNumber)
        {
            // We assume that neighborhood is always smaller than subproblems size
            size_t excees = rightIdx - partitionsNumber;
            leftIdx -= excees;
            rightIdx -= excees;
        }
        for (size_t j = leftIdx; j < rightIdx; ++j)
        {
            m_Subproblems[i].m_Neighborhood.push_back(j);
        }
    }
}

void CMOEAD::ConstructSubproblemsMultiD(size_t partitionsNumber, size_t neighborhoodSize, size_t dimCount)
{
    m_Subproblems.clear();

    // Generate subproblems
    DasDennis ddGenerator(partitionsNumber, dimCount);
    size_t subVecCount = ddGenerator.GetPointsNumber();
    ddGenerator.GeneratePoints();
    std::vector<std::vector<float>> subproblemVectors = ddGenerator.GetPoints();

    for (const auto& subVec : subproblemVectors)
    {
        m_Subproblems.emplace_back();
        SSubproblem& sp = m_Subproblems.back();
        sp.m_WeightVector = std::vector<float>(dimCount, 0.f);
        for (size_t j = 0; j < dimCount; ++j)
        {
            sp.m_WeightVector[j] = subVec[j];
        }
    }

    // Calculate distances
    using TNeighborhood = std::vector<std::pair<size_t, float>>;
    std::vector<TNeighborhood> neighborhood = std::vector<TNeighborhood>(subVecCount, TNeighborhood(subVecCount));
    for (size_t i = 0; i < subVecCount; ++i)
    {
        auto& weightVectorI = m_Subproblems[i].m_WeightVector;
        size_t entryIdx = 0;
        for (size_t j = 0; j < subVecCount; ++j)
        {
            auto& weightVectorJ = m_Subproblems[j].m_WeightVector;
            std::pair<size_t, float>& entry = neighborhood[i][entryIdx];
            entry.first = j;

            float dist = 0.f;
            if (i != j)
            {
                for (size_t d = 0; d < dimCount; ++d)
                {
                    dist += powf(weightVectorI[d] - weightVectorJ[d], 2);
                }
                dist = sqrtf(dist);
            }
            entry.second = dist;
            ++entryIdx;
        }
    }

    // Link neighborhood
    for (TNeighborhood& nh : neighborhood)
    {
        // Sort ascending by distance, including distance to self
        std::sort(nh.begin(), nh.end(), [](const std::pair<size_t, float>& a, const std::pair<size_t, float>& b) -> bool
        {
            return a.second < b.second;
        });
    }

    for (size_t i = 0; i < subVecCount; ++i)
    {
        for (size_t j = 0; j < neighborhoodSize; ++j)
        {
            m_Subproblems[i].m_Neighborhood.push_back(neighborhood[i][j].first);
        }
    }
}

void CMOEAD::EvolveToNextGeneration()
{
// Temp individual used for testing new child genes
    SMOIndividual* testIndividual = nullptr;

    for (size_t i = 0; i < m_Population.size(); ++i)
    {
        const SSubproblem& sp = m_Subproblems[i];
        const size_t nhSize = sp.m_Neighborhood.size();
        
        size_t firstParentIdx = sp.m_Neighborhood[CRandom::GetInt(0, nhSize)];
        size_t secondParentIdx = sp.m_Neighborhood[CRandom::GetInt(0,nhSize)];
        SMOIndividual* firstParent = m_Population[firstParentIdx];
        SMOIndividual* secondParent = m_Population[secondParentIdx];

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

        //Take only one child
        testIndividual = firstChild;
        m_Problem.Evaluate(*testIndividual);
        delete secondChild;

        // Now check if any neighborhood solution is improved
        for (size_t j : sp.m_Neighborhood)
        {
            if (IsBetterInSubproblem(testIndividual, m_Population[j], m_Subproblems[j]))
            {
                // I assume we can just copy whole individual
                *m_Population[j] = *testIndividual;
            }
        }

        ArchiveUtils::CopyToArchiveWithFiltering(testIndividual, m_Archive);
    }
}

bool CMOEAD::IsBetterInSubproblem(SMOIndividual *newIndividual, SMOIndividual *&oldIndividual, CMOEAD::SSubproblem &subproblem)
{
    // Simple weighted sum
    float newValue = 0.f;
    float oldValue = 0.f;
    const size_t objCount = m_Problem.GetProblemEncoding().m_objectivesNumber;
    for (size_t i = 0; i < objCount; ++i)
    {
        newValue += newIndividual->m_NormalizedEvaluation[i] * subproblem.m_WeightVector[i];
        oldValue += oldIndividual->m_NormalizedEvaluation[i] * subproblem.m_WeightVector[i];
    }
    // Minimization
    return newValue < oldValue;
}

