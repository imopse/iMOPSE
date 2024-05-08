#include "CALNS.h"

#include "../../../../utils/random/CRandom.h"
#include "../utils/experiment/CSOExperimentUtils.h"
#include "../utils/aggregatedFitness/CAggregatedFitness.h"
#include "../../../../utils/logger/ErrorUtils.h"
#include "../../../../utils/logger/CExperimentLogger.h"
#include "../../MO/utils/archive/ArchiveUtils.h"

CALNS::CALNS(
    std::vector<float>& objectiveWeights,
    AProblem& evaluator,
    AInitialization& initialization,
    SConfigMap* configMap,
    int objectiveIndex,
    bool logProgress,
    std::vector<AMutation*>& alnsRemovalMutations,
    std::vector<AMutation*>& alnsInsertionMutations
) : ASOMethod(evaluator, initialization, objectiveWeights), m_objectiveIndex(objectiveIndex), m_alnsInsertionMutations(alnsInsertionMutations), m_alnsRemovalMutations(alnsRemovalMutations), m_logProgress(logProgress)
{
    configMap->TakeValue("ALNSIterations", m_ALNSIterations);
    ErrorUtils::LowerThanZeroI("ALNS", "ALNSIterations", m_ALNSIterations);

    configMap->TakeValue("ALNSNoImprovementIterations", m_ALNSNoImprovementIterations);
    ErrorUtils::LowerThanZeroI("ALNS", "ALNSNoImprovementIterations", m_ALNSNoImprovementIterations);

    configMap->TakeValue("ALNSProbabilityStepsIterations", m_ALNSProbabilityStepsIterations);
    ErrorUtils::LowerThanZeroI("ALNS", "ALNSProbabilityStepsIterations", m_ALNSProbabilityStepsIterations);

    configMap->TakeValue("ALNSStartTemperature", m_ALNSStartTemperature);
    ErrorUtils::LowerThanZeroI("ALNS", "ALNSStartTemperature", m_ALNSStartTemperature);

    configMap->TakeValue("ALNSTemperatureAnnealingRate", m_ALNSTemperatureAnnealingRate);
    ErrorUtils::LowerThanZeroI("ALNS", "ALNSTemperatureAnnealingRate", m_ALNSTemperatureAnnealingRate);
}

void CALNS::RunOptimization()
{
    auto individual = m_Initialization.CreateMOIndividualForECVRPTW(m_Problem.GetProblemEncoding(), (CECVRPTW&)m_Problem);
    auto best = RunALNS(*individual);
    m_Problem.Evaluate(*best);

    CExperimentLogger::LogProgress(1);
    ArchiveUtils::LogParetoFront(std::vector<SMOIndividual*> { best });
    m_Problem.LogSolution(*best);
    CExperimentLogger::LogData();
    m_Problem.LogAdditionalData();
}

SMOIndividual* CALNS::RunOptimization(SMOIndividual& individual)
{
    return RunALNS(individual);
}

bool CALNS::AcceptWorseSolution(SMOIndividual& generated, SMOIndividual& current, float temperature)
{
    return CRandom::GetFloat(0, 1) < exp((generated.m_Evaluation[m_objectiveIndex] - current.m_Evaluation[m_objectiveIndex]) / temperature);
}

SMOIndividual* CALNS::RunALNS(SMOIndividual& parent)
{
    auto* best = new SMOIndividual{ parent };
    auto* current = new SMOIndividual{ *best };
    int iteration = 1;
    int iterationsWithoutImprovement = 0;
    float temperature = 0;
    std::vector<float> removalOperatorsProbabilityDistribution(m_alnsRemovalMutations.size(), 1.0f / m_alnsRemovalMutations.size());
    std::vector<float> insertionOperatorsProbabilityDistribution(m_alnsInsertionMutations.size(), 1.0f / m_alnsInsertionMutations.size());
    std::map<AMutation*, std::tuple<float, int>> removalOperatorsScores;
    std::map<AMutation*, std::tuple<float, int>> insertOperatorsScores;
    m_Problem.Evaluate(*current);
    while (iteration < (m_ALNSIterations + 1) && iterationsWithoutImprovement < m_ALNSNoImprovementIterations)
    {
        if(m_logProgress)
            CExperimentLogger::LogProgress(iteration / (float)(m_ALNSIterations + 1));
        auto* generated = new SMOIndividual(*current);
        auto& removalOperator = m_alnsRemovalMutations[CRandom::GetWeightedInt(removalOperatorsProbabilityDistribution)];
        auto& insertOperator = m_alnsInsertionMutations[CRandom::GetWeightedInt(insertionOperatorsProbabilityDistribution)];
        removalOperator->Mutate(m_Problem.GetProblemEncoding(), *generated);
        insertOperator->Mutate(m_Problem.GetProblemEncoding(), *generated);
        m_Problem.Evaluate(*generated);
        if (generated->m_isValid)
        {
            if (generated->m_Evaluation[m_objectiveIndex] < current->m_Evaluation[m_objectiveIndex])
            {
                delete current;
                current = generated;
                iterationsWithoutImprovement = 0;
                if (current->m_Evaluation[m_objectiveIndex] < best->m_Evaluation[m_objectiveIndex])
                {
                    delete best;
                    best = new SMOIndividual{ *current };
                }
            }
            else if (AcceptWorseSolution(*generated, *current, temperature))
            {
                delete current;
                current = generated;
                iterationsWithoutImprovement++;
            }
        }
        else if (AcceptWorseSolution(*generated, *current, temperature))
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
    if(m_logProgress)
        CExperimentLogger::LogProgress(1);
    return best;
}

void CALNS::UpdateScores(SMOIndividual* current,
    SMOIndividual* best,
    AMutation*& removalOperator,
    AMutation*& insertOperator,
    std::map<AMutation*, std::tuple<float, int>>& removalOperatorsScores,
    std::map<AMutation*, std::tuple<float, int>>& insertOperatorsScores
)
{
    float scoreIncrease = (current->m_Evaluation[0] + current->m_Evaluation[1]) - (best->m_Evaluation[0] + best->m_Evaluation[1]);
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

void CALNS::UpdateProbabilityTables(std::map<AMutation*, std::tuple<float, int>>& removalOperatorsScores,
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