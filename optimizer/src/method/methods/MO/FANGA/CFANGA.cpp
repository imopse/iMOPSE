#include <algorithm>
#include <sstream>
#include "CFANGA.h"
#include "method/methods/MO/utils/archive/ArchiveUtils.h"
#include "utils/logger/ErrorUtils.h"
#include "utils/dataStructures/CCSV.h"
#include "utils/logger/CExperimentLogger.h"

CFANGA::CFANGA(AProblem& evaluator, AInitialization& initialization,
               ACrossover& crossover, AMutation& mutation, CGapSelectionByRandomDim& gapSelection,
               SConfigMap* configMap)
    : AMOGeneticMethod(evaluator, initialization, crossover, mutation), m_GapSelection(gapSelection)
    , m_AdaptiveOperatorManager(configMap, evaluator, m_Population, m_Archive)
{
    configMap->TakeValue("PopulationSize", m_PopulationSize);
    ErrorUtils::LowerThanZeroI("ANTGA", "PopulationSize", m_PopulationSize);
    m_Population.reserve(m_PopulationSize);
    m_NextPopulation.reserve(m_PopulationSize);

    configMap->TakeValue("GenerationLimit", m_GenerationLimit);
    ErrorUtils::LowerThanZeroI("ANTGA", "GenerationLimit", m_GenerationLimit);
}

CFANGA::~CFANGA()
{
}

void CFANGA::RunOptimization()
{
    //CCSV<float> m_PopulationHistory(1 + m_Problem.GetProblemEncoding().m_objectivesNumber + 5);
    //CCSV<float> m_ArchiveHistory(1 + m_Problem.GetProblemEncoding().m_objectivesNumber + 5);
    CCSV<float> m_OperatorStats(3 + (m_AdaptiveOperatorManager.GetOperatorDataCount() * 3));
    CCSV<float> m_HVStats(4);

    m_StartTime = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < m_PopulationSize; ++i)
    {
        SProblemEncoding& problemEncoding = m_Problem.GetProblemEncoding();
        auto* newInd = m_Initialization.CreateMOIndividual(problemEncoding);
        m_Problem.Evaluate(*newInd);
        m_Population.push_back(newInd);
    }

    ArchiveUtils::CopyToArchiveWithFiltering(m_Population, m_Archive);

    while (m_Generation < m_GenerationLimit)
    {
        EvolveToNextGeneration();

        int fet = (m_Generation + 1) * int(m_PopulationSize);

        //LogIndividualsToCSV(m_PopulationHistory, m_NextPopulation);
        //LogIndividualsToCSV(m_ArchiveHistory, m_Archive);
        m_AdaptiveOperatorManager.LogOperatorStatsToCSV(m_Generation, m_OperatorStats);
        LogHVStatsToCSV(m_HVStats);

        for (SMOIndividual* ind: m_Population)
        {
            delete ind;
        }
        m_Population = m_NextPopulation;
        m_NextPopulation.clear();
        m_NextPopulation.reserve(m_Population.size());
        m_Generation++;

        if (fet % 5000 == 0)
        {
            //ArchiveUtils::LogParetoFront(m_Archive, fet);
        }
    }

    ArchiveUtils::LogParetoFront(m_Archive);
    //CExperimentLogger::LogResult(m_PopulationHistory.ToStringStream().str().c_str(), "PopHist.csv");
    //CExperimentLogger::LogResult(m_ArchiveHistory.ToStringStream().str().c_str(), "ArchHist.csv");
    CExperimentLogger::LogResult(m_OperatorStats.ToStringStream().str().c_str(), "OperatorStats.csv");
    CExperimentLogger::LogResult(m_HVStats.ToStringStream().str().c_str(), "HVStats.csv");
}

void CFANGA::Reset()
{
    AMOGeneticMethod::Reset();
    m_Generation = 0;
    m_AdaptiveOperatorManager.Reset();
}

void CFANGA::EvolveToNextGeneration()
{
    //SetMultiMutationCreditBasedOnArchive();
    //SetMultiMutationRegionsCreditBasedOnArchive();

    //if (m_Generation % 100 == 0)
    {
        //m_MultiMutation->ResetAllOperatorDataButAccCalls();
        //m_MultiMutation->ResetAllOperatorData();
        //m_MultiMutation->MultAllOperatorDataBy(0.5f);
        //m_MultiMutation->MultAllOperatorDataBy(0.99f);
        //ResetAllArchiveOperatorDataButAccCalls();
    }

    const auto& parents = m_GapSelection.Select(
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

void CFANGA::CrossoverAndMutate(SMOIndividual* firstParent, SMOIndividual* secondParent)
{
    auto* firstChild = new SMOIndividual{*firstParent};
    auto* secondChild = new SMOIndividual{*secondParent};

    // Copy operator data
    firstChild->m_OperatorsData = firstParent->m_OperatorsData;
    secondChild->m_OperatorsData = secondParent->m_OperatorsData;

    m_Crossover.Crossover(
        m_Problem.GetProblemEncoding(),
        *firstParent,
        *secondParent,
        *firstChild,
        *secondChild
    );

    // Local Variant
    {
        m_AdaptiveOperatorManager.LocalAdaptiveMutation(firstChild, firstParent, secondParent);
        m_AdaptiveOperatorManager.LocalAdaptiveMutation(secondChild, secondParent, firstParent);
        // Copy information back to parents
        //firstParent->m_OperatorsData = firstChild->m_OperatorsData;
        //secondParent->m_OperatorsData = secondChild->m_OperatorsData;
    }

    // Global Variant
    //{
    //    GlobalAdaptiveMutation(firstChild, secondChild, firstParent, secondParent);
    //}

    m_NextPopulation.emplace_back(firstChild);
    m_NextPopulation.emplace_back(secondChild);
}

void CFANGA::LogIndividualsToCSV(CCSV<float>& csv, const std::vector<SMOIndividual*>& individuals) const
{
    for (const auto& ind : individuals)
    {
        std::vector<float> newRow = { (float)m_Generation };
        newRow.insert(newRow.end(), ind->m_Evaluation.begin(), ind->m_Evaluation.end());
        newRow.insert(newRow.end(), ind->m_MetaInfo.begin(), ind->m_MetaInfo.end());
        for (const auto& opData : ind->m_OperatorsData)
        {
            newRow.push_back(opData.m_Credits);
            newRow.push_back(opData.m_Calls);
        }
        csv.AddRow(std::move(newRow));
    }
}

void CFANGA::LogHVStatsToCSV(CCSV<float>& csv)
{
    int fet = (m_Generation + 1) * int(m_PopulationSize);
    // It will sort the archive
    float paretoHV = CalcHV(m_Archive, std::vector<float>{ 1.f, 1.f });
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto durationMS = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - m_StartTime);
    std::vector<float> newRow = { (float)m_Generation, paretoHV, (float)durationMS.count(), float(fet) };
    csv.AddRow(std::move(newRow));
}

// TODO - copied from ParetoAnalyzer - remove it or move somewhere else
// Calculate Hyper-volume using values as they are (either absolute or normalized), using the reference point
float CFANGA::CalcHV(std::vector<SMOIndividual*>& individuals, const std::vector<float>& refPoint)
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
