#include <algorithm>
#include "CMOEAD_FRRMAB.h"
#include "../../../../utils/random/CRandom.h"
#include "../../../../utils/logger/ErrorUtils.h"
#include "utils/dataStructures/CCSV.h"
#include "utils/logger/CExperimentLogger.h"
#include "factories/method/operators/mutation/CMutationFactory.h"

CMOEAD_FRRMAB::CMOEAD_FRRMAB(
        AProblem* evaluator,
        AInitialization* initialization,
        ACrossover* crossover,
        AMutation* mutation,
        SConfigMap* configMap
) : CMOEAD(evaluator, initialization, crossover, mutation, configMap)
{
    m_MultiMutation = CMutationFactory::CreateMultiMutation(configMap, m_Problem);

    configMap->TakeValue("SlidingWindowWidth", m_SlidingWindowWidth);
    ErrorUtils::LowerThanZeroI("CMOEAD_FRRMAB", "SlidingWindowWidth", m_SlidingWindowWidth);
}

void CMOEAD_FRRMAB::RunOptimization()
{
    CCSV<float> m_OperatorStats(3 + (m_MultiMutation->GetOperatorCount() * 3));
    CCSV<float> m_HVStats(4);

    m_StartTime = std::chrono::steady_clock::now();

    int generation = 0;

    // TODO - share initialization code with the rest of the methods
    ConstructSubproblems(m_PartitionsNumber, m_NeighbourhoodSize);
    m_PopulationSize = m_Subproblems.size();
    m_Population.reserve(m_PopulationSize);

    for (size_t i = 0; i < m_PopulationSize; ++i)
    {
        SProblemEncoding& problemEncoding = m_Problem->GetProblemEncoding();
        auto* newInd = m_Initialization->CreateMOIndividual(problemEncoding);

        m_Problem->Evaluate(*newInd);

        m_Population.push_back(newInd);
    }

    ArchiveUtils::CopyToArchiveWithFiltering(m_Population, m_Archive);
    // TODO - end

    while ( generation < m_GenerationLimit)
    {
        EvolveToNextGeneration();

        int fet = (generation + 1) * int(m_PopulationSize);

        // TODO - use functions
        {
            for (size_t i = 0; i < m_MultiMutation->GetOperatorCount(); ++i)
            {
                std::vector<float> newRow = { (float)generation, (float)fet, (float)i };
                newRow.push_back((float)m_MultiMutation->GetOperatorData(i).m_Credits);
                newRow.push_back((float)m_MultiMutation->GetOperatorData(i).m_Calls);
                newRow.push_back((float)m_MultiMutation->GetOperatorData(i).m_AccCalls);
                m_OperatorStats.AddRow(std::move(newRow));
            }
        }

        {
            // It will sort the archive
            float paretoHV = CalcHV(m_Archive, std::vector<float>{ 1.f, 1.f });
            auto currentTime =  std::chrono::steady_clock::now();
            auto durationMS = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - m_StartTime);
            std::vector<float> newRow = { (float)generation, paretoHV, (float)durationMS.count(), float(fet)  };
            m_HVStats.AddRow(std::move(newRow));
        }

        generation++;

        if (fet % 5000 == 0)
        {
            ArchiveUtils::LogParetoFront(m_Archive, fet);
        }
    }

    LogParetoFront(m_Archive);

    CExperimentLogger::LogResult(m_OperatorStats.ToStringStream().str().c_str(), "OperatorStats.csv");
    CExperimentLogger::LogResult(m_HVStats.ToStringStream().str().c_str(), "HVStats.csv");
}

void CMOEAD_FRRMAB::EvolveToNextGeneration()
{
// Temp individual used for testing new child genes
    SMOIndividual* testIndividual = nullptr;

    for (size_t i = 0; i < m_Population.size(); ++i)
    {
        const SMOEADSubproblem& sp = m_Subproblems[i];
        const size_t nhSize = sp.m_Neighborhood.size();

        size_t firstParentIdx = sp.m_Neighborhood[CRandom::GetInt(0, nhSize)];
        size_t secondParentIdx = sp.m_Neighborhood[CRandom::GetInt(0,nhSize)];
        SMOIndividual* firstParent = m_Population[firstParentIdx];
        SMOIndividual* secondParent = m_Population[secondParentIdx];

        auto *firstChild = new SMOIndividual{*firstParent};
        auto *secondChild = new SMOIndividual{*secondParent};

        m_Crossover->Crossover(
            m_Problem->GetProblemEncoding(),
            *firstParent,
            *secondParent,
            *firstChild,
            *secondChild
        );

        CAtomicOperator<AMutation>* atomicMutation = m_MultiMutation->SelectOperator();
        AMutation* mutation = atomicMutation->Get();
        mutation->Mutate(m_Problem->GetProblemEncoding(), *firstChild);
        atomicMutation->GetData().m_AccCalls += 1;

        size_t operatorId = atomicMutation->GetId();
        float FIRop = 0.f;
        firstChild->m_MetaInfo = {
            (float)operatorId,
            firstParent->m_MutationIdx.has_value() ? (float)firstParent->m_MutationIdx.value() : -1.f,
            secondParent->m_MutationIdx.has_value() ? (float)secondParent->m_MutationIdx.value() : -1.f
        };
        firstChild->m_MutationIdx = operatorId;

        //Take only one child
        delete secondChild;
        testIndividual = firstChild;
        m_Problem->Evaluate(*testIndividual);

        // Now check if any neighborhood solution is improved
        for (size_t j : sp.m_Neighborhood)
        {
            float FIR = CalcFitnessImprovementRate(testIndividual, m_Population[j], m_Subproblems[j]);
            if (FIR > 0)
            {
                // I assume we can just copy whole individual
                *m_Population[j] = *testIndividual;
                FIRop += FIR;
            }
        }

        while (m_SlidingWindow.size() >= m_SlidingWindowWidth)
        {
            m_SlidingWindow.pop_front();
        }
        m_SlidingWindow.push_back({operatorId, FIRop});

        UpdateMultiOperatorCredits();

        ArchiveUtils::CopyToArchiveWithFiltering(testIndividual, m_Archive);
    }
}

float CMOEAD_FRRMAB::CalcFitnessImprovementRate(SMOIndividual* newIndividual, SMOIndividual* oldIndividual, SMOEADSubproblem& subproblem)
{
    // Simple weighted sum
    float newValue = 0.f;
    float oldValue = 0.f;
    const size_t objCount = m_Problem->GetProblemEncoding().m_objectivesNumber;
    for (size_t i = 0; i < objCount; ++i)
    {
        newValue += newIndividual->m_NormalizedEvaluation[i] * subproblem.m_WeightVector[i];
        oldValue += oldIndividual->m_NormalizedEvaluation[i] * subproblem.m_WeightVector[i];
    }
    // Minimization
    return (oldValue - newValue) / oldValue;
}

void CMOEAD_FRRMAB::UpdateMultiOperatorCredits()
{
    //size_t opCount = m_MultiMutation->GetOperatorCount();
    m_MultiMutation->ResetAllOperatorDataButAccCalls();
    for (const auto& windowSlot : m_SlidingWindow)
    {
        auto& opData = m_MultiMutation->GetOperatorData(windowSlot.m_OperatorIdx);
        opData.m_Calls += 1;
        opData.m_Credits += windowSlot.m_OperatorFIR;
    }
}

// TODO - copied from ParetoAnalyzer - remove it or move somewhere else
// Calculate Hyper-volume using values as they are (either absolute or normalized), using the reference point
float CMOEAD_FRRMAB::CalcHV(std::vector<SMOIndividual*>& individuals, const std::vector<float>& refPoint)
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