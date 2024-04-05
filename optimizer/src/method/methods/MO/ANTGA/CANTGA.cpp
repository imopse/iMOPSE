#include <algorithm>
#include <sstream>
#include "CANTGA.h"
#include "method/methods/MO/utils/archive/ArchiveUtils.h"
#include "utils/logger/ErrorUtils.h"
#include "factories/method/CMethodFactory.h"
#include "utils/dataStructures/CCSV.h"
#include "utils/logger/CExperimentLogger.h"

CANTGA::CANTGA(AProblem& evaluator, AInitialization& initialization,
               ACrossover& crossover, AMutation& mutation, CGapSelectionByRandomDim& gapSelection,
               SConfigMap* configMap)
    : AMOGeneticMethod(evaluator, initialization, crossover, mutation), m_GapSelection(gapSelection)
{
    configMap->TakeValue("PopulationSize", m_PopulationSize);
    ErrorUtils::LowerThanZeroI("ANTGA", "PopulationSize", m_PopulationSize);
    m_Population.reserve(m_PopulationSize);
    m_NextPopulation.reserve(m_PopulationSize);

    configMap->TakeValue("GenerationLimit", m_GenerationLimit);
    ErrorUtils::LowerThanZeroI("ANTGA", "GenerationLimit", m_GenerationLimit);
    
}

CANTGA::~CANTGA()
{

}

void CANTGA::RunOptimization()
{
    m_Generation = 0;

    CCSV<float> m_PopulationHistory(1 + m_Problem.GetProblemEncoding().m_objectivesNumber + 5);
    CCSV<float> m_ArchiveHistory(1 + m_Problem.GetProblemEncoding().m_objectivesNumber + 5);

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

        LogIndividualsToCSV(m_PopulationHistory, m_NextPopulation);
        LogIndividualsToCSV(m_ArchiveHistory, m_Archive);

        for (SMOIndividual* ind: m_Population)
        {
            delete ind;
        }
        m_Population = m_NextPopulation;
        m_NextPopulation.clear();
        m_NextPopulation.reserve(m_Population.size());
        m_Generation++;
    }

    ArchiveUtils::LogParetoFront(m_Archive);
    CExperimentLogger::LogResult(m_PopulationHistory.ToStringStream().str().c_str(), "PopHist.csv");
    CExperimentLogger::LogResult(m_ArchiveHistory.ToStringStream().str().c_str(), "ArchHist.csv");
}

void CANTGA::EvolveToNextGeneration()
{
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

void CANTGA::CrossoverAndMutate(SMOIndividual* firstParent, SMOIndividual* secondParent)
{
    auto* firstChild = new SMOIndividual{*firstParent};
    auto* secondChild = new SMOIndividual{*secondParent};

    m_Crossover.Crossover(
        m_Problem.GetProblemEncoding(),
        *firstParent,
        *secondParent,
        *firstChild,
        *secondChild
    );
    

    m_Problem.Evaluate(*secondChild);

    m_NextPopulation.emplace_back(firstChild);
    m_NextPopulation.emplace_back(secondChild);
}

void CANTGA::LogIndividualsToCSV(CCSV<float>& csv, const std::vector<SMOIndividual*>& individuals) const
{
    for (const auto& ind : individuals)
    {
        std::vector<float> newRow = { (float)m_Generation };
        newRow.insert(newRow.end(), ind->m_Evaluation.begin(), ind->m_Evaluation.end());
        newRow.insert(newRow.end(), ind->m_MetaInfo.begin(), ind->m_MetaInfo.end());
        csv.AddRow(std::move(newRow));
    }
}