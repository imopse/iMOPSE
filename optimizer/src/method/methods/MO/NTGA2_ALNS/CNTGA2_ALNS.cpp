#include <algorithm>
#include <sstream>
#include "CNTGA2_ALNS.h"
#include "../utils/archive/ArchiveUtils.h"
#include "../utils/clustering/CNonDominatedSorting.h"
#include "../../../../utils/logger/ErrorUtils.h"
#include "../../../../utils/random/CRandom.h"
#include "../../../../utils/logger/CExperimentLogger.h"

CNTGA2_ALNS::CNTGA2_ALNS(AProblem &evaluator,
    AInitialization &initialization,
    CRankedTournament &rankedTournament,
    CGapSelectionByRandomDim &gapSelection,
    ACrossover &crossover,
    AMutation &mutation,
    SConfigMap *configMap,
    std::vector<AMutation*>& alnsRemovalMutations,
    std::vector<AMutation*>& alnsInsertionMutations
) :
        AMOGeneticMethod(evaluator, initialization, crossover, mutation),
        m_RankedTournament(rankedTournament),
        m_GapSelection(gapSelection),
        m_alnsRemovalMutations(alnsRemovalMutations),
        m_alnsInsertionMutations(alnsInsertionMutations)
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

    configMap->TakeValue("EffectivnessThreshold", m_effectivnessThreshold);
    ErrorUtils::LowerThanZeroF("NTGA2_ALNS", "EffectivnessThreshold", m_effectivnessThreshold);

    configMap->TakeValue("ALNSIterations", m_ALNSIterations);
    ErrorUtils::LowerThanZeroI("NTGA2_ALNS", "ALNSIterations", m_ALNSIterations);

    configMap->TakeValue("ALNSNoImprovementIterations", m_ALNSNoImprovementIterations);
    ErrorUtils::LowerThanZeroI("NTGA2_ALNS", "ALNSNoImprovementIterations", m_ALNSNoImprovementIterations);

    configMap->TakeValue("ALNSProbabilityStepsIterations", m_ALNSProbabilityStepsIterations);
    ErrorUtils::LowerThanZeroI("NTGA2_ALNS", "ALNSProbabilityStepsIterations", m_ALNSProbabilityStepsIterations);

    configMap->TakeValue("ALNSStartTemperature", m_ALNSStartTemperature);
    ErrorUtils::LowerThanZeroI("NTGA2_ALNS", "ALNSStartTemperature", m_ALNSStartTemperature);

    configMap->TakeValue("ALNSTemperatureAnnealingRate", m_ALNSTemperatureAnnealingRate);
    ErrorUtils::LowerThanZeroI("NTGA2_ALNS", "ALNSTemperatureAnnealingRate", m_ALNSTemperatureAnnealingRate);

    configMap->TakeValue("ALNSProbabilityPercent", m_ALNSProbabilityPercent);
    ErrorUtils::OutOfScopeF("NTGA2_ALNS", "ALNSProbabilityPercent", m_ALNSProbabilityPercent / 100.f);
}

void CNTGA2_ALNS::RunOptimization()
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
        if ((generation % 100) < (100 - m_GapSelectionPercent))
        {
            RunGeneration();
        }
        else
        {
            RunGenerationWithGap();
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

    bool shouldUseALNS = ShouldUseALNS(m_PreviousPopulation, m_Population);

    for (size_t i = 0; i < m_PopulationSize; i += 2)
    {
        auto* firstParent = m_RankedTournament.Select(m_Population);
        auto* secondParent = m_RankedTournament.Select(m_Population);

        if (shouldUseALNS && CRandom::GetInt(0, 101) < m_ALNSProbabilityPercent) {
            auto* firstChild = RunALNS(*firstParent);
            auto* secondChild = RunALNS(*secondParent);
            EvaluateAndAdd(*firstChild);
            EvaluateAndAdd(*secondChild);
        }
        else {
            CrossoverAndMutate(*firstParent, *secondParent);
        }
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
        if (shouldUseALNS && CRandom::GetInt(0, 101) < m_ALNSProbabilityPercent) {
            auto* firstChild = RunALNS(*parentsPair.first);
            auto* secondChild = RunALNS(*parentsPair.second);
            EvaluateAndAdd(*firstChild);
            EvaluateAndAdd(*secondChild);
        }
        else {
            CrossoverAndMutate(*parentsPair.first, *parentsPair.second);
        }
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
    for (int j = 0; j < previousPopulation.size(); j++) {
        evaluationPoints += previousPopulation[j]->m_Evaluation[0];
    }
    previousAverageEvaluation += evaluationPoints / m_PreviousPopulation.size();
    evaluationPoints = 0;
    for (int j = 0; j < currentPopulation.size(); j++) {
        evaluationPoints += currentPopulation[j]->m_Evaluation[0];
    }
    currentAverageEvaluation += evaluationPoints / m_PreviousPopulation.size();
    if ((currentAverageEvaluation / previousAverageEvaluation) - 1 < m_effectivnessThreshold) {
        return true;
    }
    return false;
}

bool CNTGA2_ALNS::AcceptWorseSolution(SMOIndividual& generated, SMOIndividual& current, float temperature)
{
    return CRandom::GetFloat(0, 1) < exp((generated.m_Evaluation[0] - current.m_Evaluation[0]) / temperature);
}

SMOIndividual* CNTGA2_ALNS::RunALNS(SMOIndividual& parent) 
{
    auto* best = new SMOIndividual{ parent };
    auto* current = new SMOIndividual{ *best };
    int iteration = 1;
    int iterationsWithoutImprovement = 0;
    float temperature = 0;
    std::vector<float> removalOperatorsProbabilityDistribution(m_alnsRemovalMutations.size(), 1.0f/m_alnsRemovalMutations.size());
    std::vector<float> insertionOperatorsProbabilityDistribution(m_alnsInsertionMutations.size(), 1.0f / m_alnsInsertionMutations.size());
    std::map<AMutation*, std::tuple<float, int>> removalOperatorsScores;
    std::map<AMutation*, std::tuple<float, int>> insertOperatorsScores;
    m_Problem.Evaluate(*current);
    while (iteration < (m_ALNSIterations + 1) && iterationsWithoutImprovement < m_ALNSNoImprovementIterations) 
    {
        auto* generated = new SMOIndividual(*current);
        auto& removalOperator = m_alnsRemovalMutations[CRandom::GetWeightedInt(removalOperatorsProbabilityDistribution)];
        auto& insertOperator = m_alnsInsertionMutations[CRandom::GetWeightedInt(insertionOperatorsProbabilityDistribution)];
        removalOperator->Mutate(m_Problem.GetProblemEncoding(), *generated);
        insertOperator->Mutate(m_Problem.GetProblemEncoding(), *generated);
        m_Problem.Evaluate(*generated);
        if (generated->m_isValid) 
        {
            if (generated->m_Evaluation[2] < current->m_Evaluation[2]) 
            {
                delete current;
                current = generated;
                iterationsWithoutImprovement = 0;
                if (current->m_Evaluation[2] < best->m_Evaluation[2]) 
                {
                    delete best;
                    best = new SMOIndividual{ *current };
                }
            }
            else if(AcceptWorseSolution(*generated, *current, temperature)) 
            {
                delete current;
                current = generated;
                iterationsWithoutImprovement++;
            }
        }
        else if(AcceptWorseSolution(*generated, *current, temperature)) 
        {
            delete current;
            current = generated;
            iterationsWithoutImprovement++;
        }
        else 
        {
            iterationsWithoutImprovement++;
        }

        UpdateScores(current, 
            best, 
            removalOperator, 
            insertOperator, 
            removalOperatorsScores, 
            insertOperatorsScores
        );

        if (iteration % m_ALNSProbabilityStepsIterations == 0)
        {
            UpdateProbabilityTables(removalOperatorsScores,
                removalOperatorsProbabilityDistribution,
                insertOperatorsScores,
                insertionOperatorsProbabilityDistribution
            );
        }

        iteration++;      

        temperature = temperature * m_ALNSTemperatureAnnealingRate;
    }
    return best;
}

void CNTGA2_ALNS::UpdateScores(SMOIndividual* current, 
    SMOIndividual* best, 
    AMutation*& removalOperator,
    AMutation*& insertOperator,
    std::map<AMutation*, std::tuple<float, int>>& removalOperatorsScores,
    std::map<AMutation*, std::tuple<float, int>>& insertOperatorsScores
) 
{
    float scoreIncrease = current->m_Evaluation[2] - best->m_Evaluation[2];
    if (removalOperatorsScores.count(removalOperator))
    {
        removalOperatorsScores[removalOperator] = std::tuple<float, int>(std::get<0>(removalOperatorsScores[removalOperator]) + scoreIncrease, ++std::get<1>(removalOperatorsScores[removalOperator]));
    }
    else
    {
        removalOperatorsScores.emplace(removalOperator, std::tuple<float, int>(scoreIncrease, 1));
    }
    if (insertOperatorsScores.count(insertOperator))
    {
        insertOperatorsScores[insertOperator] = std::tuple<float, int>(std::get<0>(insertOperatorsScores[insertOperator]) + scoreIncrease, ++std::get<1>(insertOperatorsScores[insertOperator]));
    }
    else
    {
        insertOperatorsScores.emplace(insertOperator, std::tuple<float, int>(scoreIncrease, 1));
    }
}

void CNTGA2_ALNS::UpdateProbabilityTables(std::map<AMutation*, std::tuple<float, int>>& removalOperatorsScores, 
    std::vector<float>& removalOperatorsProbabilityDistribution,
    std::map<AMutation*, std::tuple<float, int>>& insertOperatorsScores,
    std::vector<float>& insertionOperatorsProbabilityDistribution
)
{
    float probabilityChange = CRandom::GetFloat(0.01f, 0.05f);
    if (removalOperatorsScores.size() >= 2) {
        using pair_type = std::map<AMutation*, std::tuple<float, int>>::value_type;
        auto best = std::min_element(removalOperatorsScores.begin(), removalOperatorsScores.end(), [](const pair_type& p1, const pair_type& p2)
            {
                return std::get<0>(p1.second) / std::get<1>(p1.second) < std::get<0>(p2.second) / std::get<1>(p2.second);
            });
        auto worst = std::max_element(removalOperatorsScores.begin(), removalOperatorsScores.end(), [](const pair_type& p1, const pair_type& p2)
            {
                return std::get<0>(p1.second) / std::get<1>(p1.second) < std::get<0>(p2.second) / std::get<1>(p2.second);
            });
        removalOperatorsProbabilityDistribution[std::find(m_alnsRemovalMutations.begin(), m_alnsRemovalMutations.end(), best->first) - m_alnsRemovalMutations.begin()] += probabilityChange;
        removalOperatorsProbabilityDistribution[std::find(m_alnsRemovalMutations.begin(), m_alnsRemovalMutations.end(), worst->first) - m_alnsRemovalMutations.begin()] -= probabilityChange;
    }
    if (insertOperatorsScores.size() >= 2) {
        using pair_type = std::map<AMutation*, std::tuple<float, int>>::value_type;
        auto best = std::min_element(insertOperatorsScores.begin(), insertOperatorsScores.end(), [](const pair_type& p1, const pair_type& p2)
            {
                return std::get<0>(p1.second) / std::get<1>(p1.second) < std::get<0>(p2.second) / std::get<1>(p2.second);
            });
        auto worst = std::max_element(insertOperatorsScores.begin(), insertOperatorsScores.end(), [](const pair_type& p1, const pair_type& p2)
            {
                return std::get<0>(p1.second) / std::get<1>(p1.second) < std::get<0>(p2.second) / std::get<1>(p2.second);
            });
        insertionOperatorsProbabilityDistribution[std::find(m_alnsInsertionMutations.begin(), m_alnsInsertionMutations.end(), best->first) - m_alnsInsertionMutations.begin()] += probabilityChange;
        insertionOperatorsProbabilityDistribution[std::find(m_alnsInsertionMutations.begin(), m_alnsInsertionMutations.end(), worst->first) - m_alnsInsertionMutations.begin()] -= probabilityChange;
    }
}

void CNTGA2_ALNS::LogResult()
{
    ArchiveUtils::LogParetoFront(m_Archive);
    for (int i = 0; i < m_Archive.size(); i++) {
        m_Problem.LogSolution(*m_Archive[i]);
    }
    CExperimentLogger::LogData();
}