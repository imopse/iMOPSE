#include "../../../../../utils/fileReader/CReadUtils.h"
#include "../../CAggregatedFitness.h"
#include "../../../../../utils/logger/CExperimentLogger.h"
#include "../../../../../utils/random/CRandom.h"
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <iostream>
#include "CACO_TSP.h"

CACO_TSP::CACO_TSP(
        AProblem &evaluator,
        AInitialization &initialization,
        SConfigMap *configMap,
        std::vector<float> &objectiveWeights
) : CACO(evaluator, initialization,objectiveWeights) {
    configMap->TakeValue("GenerationLimit", m_GenerationLimit);
    configMap->TakeValue("ReducingMultiplier", m_ReducingMultiplier);
    configMap->TakeValue("PopulationSize", m_PopulationSize);
    std::string  initType ;
    configMap->TakeValue("InitType", initType);
    if(initType=="Uniform"){
        m_InitType=Uniform;
    }
    if(initType=="Distance"){
        m_InitType=Distance;
    }

    m_Population.reserve(m_PopulationSize);

    size_t numberOfCities = m_Problem.GetProblemEncoding().m_Encoding[0].m_SectionDescription.size();
    m_PheromoneMap = std::vector<std::vector<float>>(numberOfCities, std::vector<float>(numberOfCities, 0.0));

    m_DistanceMatrix = m_Problem.GetProblemEncoding().m_additionalProblemData;

}

void CACO_TSP::SavePheromoneMap(int generation) {
    std::filesystem::path baseDirPath("../sptest/map/");
    if (!std::filesystem::exists(baseDirPath)) {
        std::filesystem::create_directories(baseDirPath);
    }
    std::ofstream outFile("../sptest/map/"+std::to_string(generation) + ".txt");
    if (!outFile.is_open()) {
        std::cerr << "Error opening file: " << generation << std::endl;
        return;
    }

    for (const auto& row : m_PheromoneMap) {
        for (const float& value : row) {
            outFile << value << ' ';
        }
        outFile << '\n';
    }

    outFile.close();
}

void CACO_TSP::RunOptimization() {
    int generation = 0;
    ResetPheromoneMap();
    m_GloballyBest=GetRandomAnt();

    RandomAnts();
    LeavePheromone();
    AddExperimentData(generation);

    while (generation < m_GenerationLimit){
        RunAnts();
        GetBestRoute();
        AddExperimentData(generation);
        generation++;
        LeavePheromone();
    }

    m_Population.push_back(m_GloballyBest);
    m_GloballyBest= nullptr;
    AddExperimentData(generation);

    CExperimentLogger::LogData();
    LogResultData();
}

void CACO_TSP::GetBestRoute() {
    SProblemEncoding& problemEncoding = m_Problem.GetProblemEncoding();
    SGenotype newGenotype;

    auto numberOfCities = problemEncoding.m_Encoding[0].m_SectionDescription.size();
    auto start = 0;
    newGenotype.m_IntGenotype.push_back(start);
    auto currentPosition = start;
    std::vector<int> visited;
    visited.push_back(currentPosition);

    for (int i = 1; i < numberOfCities; i++) {
        float maxEvaluation = std::numeric_limits<float>::min();
        int nextPosition;
        for (int ii = 0; ii < numberOfCities; ii++) {
            auto found_element = std::find(visited.begin(), visited.end(), ii);
            if (found_element != visited.end()) {
                continue;
            }
            if (ii == currentPosition) {
                continue;
            }
            if (m_PheromoneMap[currentPosition][ii] > maxEvaluation) {
                maxEvaluation = m_PheromoneMap[currentPosition][ii];
                nextPosition = ii;
            }
        }

        newGenotype.m_IntGenotype.push_back(nextPosition);
        visited.push_back(nextPosition);
        currentPosition = nextPosition;
    }

    auto *newAnt = m_Initialization.CreateSOIndividual(problemEncoding, newGenotype);
    m_Problem.Evaluate(*newAnt);
    CAggregatedFitness::CountFitness(*newAnt, m_ObjectiveWeights);

    m_Population.push_back(newAnt);
}


void CACO_TSP::LeavePheromone() {
    // Find min and max evaluation for normalization
    float maxEvaluation = std::numeric_limits<float>::min();
    float minEvaluation = std::numeric_limits<float>::max();
    for (auto *ant: m_Population) {
        float evaluation = ant->m_Fitness;
        if (evaluation > maxEvaluation) {
            maxEvaluation = evaluation;
        }
        if (evaluation < minEvaluation) {
            minEvaluation = evaluation;
        }
    }

    // Add normalized pheromone to the pheromone path
    for (auto *ant: m_Population) {
        float evaluation = ant->m_Fitness;
        float pheromone_delta = (evaluation - minEvaluation) / (maxEvaluation - minEvaluation);

        auto sequence = ant->m_Genotype.m_IntGenotype;
        for (int i = 0; i < sequence.size() - 1; i++) {
            float distance = m_DistanceMatrix[sequence[i]][sequence[i + 1]];
            pheromone_delta /= distance;
            m_PheromoneMap[sequence[i]][sequence[i + 1]] += pheromone_delta;
            m_PheromoneMap[sequence[i + 1]][sequence[i]] += pheromone_delta;
        }
        m_PheromoneMap[sequence[sequence.size() - 1]][sequence[0]] += pheromone_delta;
        m_PheromoneMap[sequence[0]][sequence[sequence.size() - 1]] += pheromone_delta;
    }

    // Reduce pheromone on all paths
    auto size = m_Population[0]->m_Genotype.m_IntGenotype.size();
    for (int i = 0; i < size; i++) {
        for (int ii = i + 1; ii < size; ii++) {
            m_PheromoneMap[i][ii] *= m_ReducingMultiplier;
            m_PheromoneMap[ii][i] = m_PheromoneMap[i][ii];
        }
    }
}

SSOIndividual* CACO_TSP::GetRandomAnt(){
    SProblemEncoding& problemEncoding = m_Problem.GetProblemEncoding();
    auto* newAnt = m_Initialization.CreateSOIndividual(problemEncoding);

    m_Problem.Evaluate(*newAnt);
    CAggregatedFitness::CountFitness(*newAnt, m_ObjectiveWeights);

    return newAnt;
}

void CACO_TSP::RandomAnts() {
    for (size_t i = 0; i < m_PopulationSize; ++i) {
        auto newAnt= GetRandomAnt();
        m_Population.push_back(newAnt);
    }
}

void CACO_TSP::AntMarch() {
    SProblemEncoding& problemEncoding = m_Problem.GetProblemEncoding();
    SGenotype newGenotype;

    auto numberOfCities = problemEncoding.m_Encoding[0].m_SectionDescription.size();
    auto randomStart = CRandom::GetInt(0, numberOfCities);
    newGenotype.m_IntGenotype.push_back(randomStart);
    auto currentPosition = randomStart;
    std::vector<int> visited;
    visited.push_back(currentPosition);


    float conformism = CRandom::GetFloat(0, 1);

    for (int i = 1; i < numberOfCities; i++) {
        std::vector<std::pair<int, float>> best_routes;
        for (int ii = 0; ii < numberOfCities; ii++) {
            auto found_element = std::find(visited.begin(), visited.end(), ii);
            if (found_element != visited.end()) {
                continue;
            }
            if (ii == currentPosition) {
                continue;
            }
            best_routes.emplace_back(
                    ii, m_PheromoneMap[currentPosition][ii]
            );
        }
        std::sort(best_routes.begin(), best_routes.end(),
                  [](const std::pair<int, float> &a, const std::pair<int, float> &b) {
                      return a.second > b.second;
                  });

        float probability = 0.8;
        int nextPosition = best_routes[0].first;
        for (auto pair: best_routes) {
            if (conformism < probability) {
                nextPosition = pair.first;
                break;
            }
            probability+=0.1;
        }

        newGenotype.m_IntGenotype.push_back(nextPosition);
        visited.push_back(nextPosition);
        currentPosition = nextPosition;
    }


    auto *newAnt = m_Initialization.CreateSOIndividual(problemEncoding, newGenotype);
    m_Problem.Evaluate(*newAnt);
    CAggregatedFitness::CountFitness(*newAnt, m_ObjectiveWeights);

    m_Population.push_back(newAnt);

}

void CACO_TSP::RunAnts() {
    int bestAntPosition=-1;
    for (int i=0; i<m_PopulationSize;i++) {
        if (m_Population[i]->m_Fitness < m_GloballyBest->m_Fitness) {
            bestAntPosition = i;
        }
    }
    if(bestAntPosition!=-1){
        m_GloballyBest=m_Population[bestAntPosition];
        m_Population[bestAntPosition]= nullptr;
    }

    ASOMethod::Reset();
    for (size_t i = 0; i < m_PopulationSize; ++i) {
        AntMarch();
    }
}

void CACO_TSP::ResetPheromoneMap(){
    size_t numberOfCities = m_Problem.GetProblemEncoding().m_Encoding[0].m_SectionDescription.size();
    m_PheromoneMap = std::vector<std::vector<float>>(numberOfCities, std::vector<float>(numberOfCities, 0.0));

    m_DistanceMatrix = m_Problem.GetProblemEncoding().m_additionalProblemData;

    for(int i=0;i< m_PheromoneMap.size();i++){
        for(int ii=0;ii< m_PheromoneMap.size();ii++){
            if(i == ii ){
                m_PheromoneMap[i][ii]=0;
            }else{
                switch(m_InitType){
                    case Uniform:
                        m_PheromoneMap[i][ii]=1;
                    case Distance:
                        m_PheromoneMap[i][ii]=1/m_DistanceMatrix[i][ii];
                    default:
                        m_PheromoneMap[i][ii]=1;

                }
            }
        }
    }

}

void CACO_TSP::AddExperimentData(int generation) {
    SSOIndividual *best = *std::min_element(m_Population.begin(), m_Population.end(),
                                            [](const auto &a, const auto &b) {
                                                return a->m_Fitness < b->m_Fitness;
                                            });
    SSOIndividual *worst = *std::max_element(m_Population.begin(), m_Population.end(),
                                             [](const auto &a, const auto &b) {
                                                 return a->m_Fitness < b->m_Fitness;
                                             });

    float totalFitness = 0.0;
    for (const auto &individual: m_Population) {
        totalFitness += individual->m_Fitness;
    }
    float meanFitness = totalFitness / float(m_Population.size());

    std::string generationData = std::to_string(generation) + ';' +
                                 std::to_string(best->m_Fitness) + ';' +
                                 std::to_string(worst->m_Fitness) + ';' +
                                 std::to_string(meanFitness);
    CExperimentLogger::AddLine(generationData.c_str());
}

void CACO_TSP::LogResultData() {
    SSOIndividual *best = *std::min_element(m_Population.begin(), m_Population.end(),
                                            [](const auto &a, const auto &b) {
                                                return a->m_Fitness < b->m_Fitness;
                                            });
    CExperimentLogger::LogResult(std::to_string(best->m_Fitness).c_str());
}




