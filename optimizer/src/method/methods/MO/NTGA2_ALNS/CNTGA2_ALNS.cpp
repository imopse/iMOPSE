#include <algorithm>
#include <sstream>
#include "CNTGA2_ALNS.h"
#include "../../SO/ALNS/CALNS.h"
#include "../utils/archive/ArchiveUtils.h"
#include "../utils/clustering/CNonDominatedSorting.h"
#include "../../../../utils/logger/ErrorUtils.h"
#include "../../../../utils/random/CRandom.h"
#include "../../../../utils/logger/CExperimentLogger.h"

CNTGA2_ALNS::CNTGA2_ALNS(AProblem &evaluator,
    AInitialization &initialization,
    CRankedTournament &rankedTournament,
    CRankedTournament& alnsRankedTournament,
    CGapSelectionByRandomDim &gapSelection,
    ACrossover &crossover,
    AMutation &mutation,
    SConfigMap *configMap,
    std::vector<CALNS*>& alnsInstances
) :
        AMOGeneticMethod(evaluator, initialization, crossover, mutation),
        m_RankedTournament(rankedTournament),
        m_GapSelection(gapSelection),
        m_ALNSInstances(alnsInstances),
        m_AlnsRankedTournament(alnsRankedTournament)
{
    configMap->TakeValue("GapSelectionPercent", m_GapSelectionPercent);
    ErrorUtils::OutOfScopeF("NTGA2_ALNS", "GapSelectionPercent", m_GapSelectionPercent / 100.f); // Assuming this checks for a valid percentage range

    configMap->TakeValue("PopulationSize", m_PopulationSize);
    ErrorUtils::LowerThanZeroI("NTGA2_ALNS", "PopulationSize", m_PopulationSize);
    m_Population.reserve(m_PopulationSize);
    m_NextPopulation.reserve(m_PopulationSize);
    m_PreviousPopulation.reserve(m_PopulationSize);

    configMap->TakeValue("GenerationLimit", m_GenerationLimit);
    ErrorUtils::LowerThanZeroI("NTGA2_ALNS", "GenerationLimit", m_GenerationLimit);

    configMap->TakeValue("EliteSize", m_EliteSize);
    ErrorUtils::LowerThanZeroI("NTGA2_ALNS", "EliteSize", m_EliteSize);
}

void CNTGA2_ALNS::RunOptimization()
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
        CExperimentLogger::LogProgress(generation / (float)m_GenerationLimit);
        if ((generation % 100) < (100 - m_GapSelectionPercent))
        {
            RunGeneration();
        }
        else
        {
            RunGenerationWithGap();
        }

        for (size_t i = 0; i < m_EliteSize; i++) {
            auto* parent = m_AlnsRankedTournament.Select(m_NextPopulation);
            m_NextPopulation.erase(std::find(m_NextPopulation.begin(), m_NextPopulation.end(), parent));
            auto* newIndividual = RunALNS(*parent);
            delete parent;
            m_NextPopulation.push_back(newIndividual);
        }

        ArchiveUtils::CopyToArchiveWithFiltering(m_NextPopulation, m_Archive);

        for (SMOIndividual *ind: m_PreviousPopulation)
        {
            delete ind;
        }
        m_PreviousPopulation = m_Population;
        m_Population = m_NextPopulation;
        m_NextPopulation.clear();
        m_NextPopulation.reserve(m_Population.size());

        ++generation;
    }
    CExperimentLogger::LogProgress(1);
    LogResult();
}

void CNTGA2_ALNS::RunGeneration()
{
    std::vector<SMOIndividual*> parentsVector;
    parentsVector.reserve(m_Archive.size() + m_Population.size());
    parentsVector.insert(parentsVector.end(), m_Archive.begin(), m_Archive.end());
    parentsVector.insert(parentsVector.end(), m_Population.begin(), m_Population.end());

    CNonDominatedSorting nonDominatedSorting;
    std::vector<std::vector<size_t>> combinedClusters;
    nonDominatedSorting.Cluster(parentsVector, combinedClusters);

    for (size_t i = 0; i < m_PopulationSize; i += 2)
    {
        auto* firstParent = m_RankedTournament.Select(m_Population);
        auto* secondParent = m_RankedTournament.Select(m_Population);

        CrossoverAndMutate(*firstParent, *secondParent);
    }
}

void CNTGA2_ALNS::RunGenerationWithGap()
{
    const auto& parents = m_GapSelection.Select(
        m_Archive,
        m_Problem.GetProblemEncoding().m_objectivesNumber,
        m_PopulationSize
    );
    bool shouldUseALNS = ShouldUseALNS(m_PreviousPopulation, m_Population);

    for (auto parentsPair : parents) {
        CrossoverAndMutate(*parentsPair.first, *parentsPair.second);
    }
}

void CNTGA2_ALNS::CrossoverAndMutate(SMOIndividual &firstParent, SMOIndividual &secondParent)
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

    EvaluateAndAdd(*firstChild);
    EvaluateAndAdd(*secondChild);
}

void CNTGA2_ALNS::EvaluateAndAdd(SMOIndividual& individual)
{
    m_Problem.Evaluate(individual);
    m_NextPopulation.emplace_back(&individual);
}

bool CNTGA2_ALNS::ShouldUseALNS(std::vector<SMOIndividual*>& previousPopulation, std::vector<SMOIndividual*> currentPopulation) 
{
    float previousAverageEvaluation = 0;
    float currentAverageEvaluation = 0;
    float evaluationPoints = 0;
    for (int i = 0; i < previousPopulation.size(); i++) {
        for (int j = 0; j < previousPopulation[i]->m_Evaluation.size(); j++) {
            evaluationPoints += previousPopulation[i]->m_Evaluation[j];
        }
    }
    previousAverageEvaluation += evaluationPoints / m_PreviousPopulation.size();
    evaluationPoints = 0;
    for (int i = 0; i < currentPopulation.size(); i++) {
        for (int j = 0; j < currentPopulation[i]->m_Evaluation.size(); j++) {
            evaluationPoints += currentPopulation[i]->m_Evaluation[j];
        }
    }
    currentAverageEvaluation += evaluationPoints / m_PreviousPopulation.size();
    if ((currentAverageEvaluation / previousAverageEvaluation) - 1 < 1) {
        return true;
    }
    return false;
}


SMOIndividual* CNTGA2_ALNS::RunALNS(SMOIndividual& parent) 
{
    int instanceToUse = CRandom::GetInt(0, m_ALNSInstances.size());
    return m_ALNSInstances[instanceToUse]->RunOptimization(parent);
}

void CNTGA2_ALNS::LogResult()
{
    ArchiveUtils::LogParetoFront(m_Archive);
    for (int i = 0; i < m_Archive.size(); i++) {
        m_Problem.LogSolution(*m_Archive[i]);
    }
    CExperimentLogger::LogData();
    m_Problem.LogAdditionalData();
}