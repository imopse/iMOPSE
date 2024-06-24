#include "CSOExperimentUtils.h"
#include "../../../../../utils/logger/CExperimentLogger.h"
#include <algorithm>
#include <string>
#include <sstream>

void CSOExperimentUtils::AddExperimentData(const int generation, const std::vector<SSOIndividual*>& population)
{
    auto* best = *std::min_element(population.begin(), population.end(),
                                            [](const auto &a, const auto &b)
                                            {
                                                return a->m_Fitness < b->m_Fitness;
                                            });
    auto* worst = *std::max_element(population.begin(), population.end(),
                                             [](const auto &a, const auto &b)
                                             {
                                                 return a->m_Fitness < b->m_Fitness;
                                             });

    float totalFitness = 0.0;
    for (const auto& individual: population)
    {
        totalFitness += individual->m_Fitness;
    }
    float meanFitness = totalFitness / float(population.size());

    std::string generationData = std::to_string(generation) + ';' +
                                 std::to_string(best->m_Fitness) + ';' +
                                 std::to_string(worst->m_Fitness) + ';' +
                                 std::to_string(meanFitness);
    CExperimentLogger::AddLine(generationData.c_str());
}

void CSOExperimentUtils::AddExperimentData(int generation, const std::vector<SParticle *> &swarm)
{
    auto* best = *std::min_element(swarm.begin(), swarm.end(),
                                   [](const auto &a, const auto &b)
                                   {
                                       return a->m_Fitness < b->m_Fitness;
                                   });
    auto* worst = *std::max_element(swarm.begin(), swarm.end(),
                                    [](const auto &a, const auto &b)
                                    {
                                        return a->m_Fitness < b->m_Fitness;
                                    });

    float totalFitness = 0.0;
    for (const auto& individual: swarm)
    {
        totalFitness += individual->m_Fitness;
    }
    float meanFitness = totalFitness / float(swarm.size());

    std::string generationData = std::to_string(generation) + ';' +
                                 std::to_string(best->m_Fitness) + ';' +
                                 std::to_string(worst->m_Fitness) + ';' +
                                 std::to_string(meanFitness);
    CExperimentLogger::AddLine(generationData.c_str());
}

SSOIndividual* CSOExperimentUtils::FindBest(const std::vector<SSOIndividual *> &population)
{
    return *std::min_element(population.begin(), population.end(),
                      [](const auto &a, const auto &b)
                      {
                          return a->m_Fitness < b->m_Fitness;
                      });
}

SSOIndividual* CSOExperimentUtils::FindBest(const std::vector<SParticle *> &swarm)
{
    return *std::min_element(swarm.begin(), swarm.end(),
                             [](const auto &a, const auto &b)
                             {
                                 return a->m_Fitness < b->m_Fitness;
                             });
}

void CSOExperimentUtils::LogResultData(SSOIndividual& best, AProblem& problem)
{
    CExperimentLogger::LogData("experiment.csv");

    std::string resultString = BestToCSVString(best);
    CExperimentLogger::LogResult(resultString.c_str());
    problem.LogSolution(best);
}

std::string CSOExperimentUtils::BestToCSVString(const SSOIndividual &best)
{
    std::ostringstream oss;
    oss << best.m_Fitness; // Append the fitness

    // Append each element of the evaluation vector
    for (const auto& value : best.m_NormalizedEvaluation) {
        oss << ";" << value;
    }

    return oss.str();
}
