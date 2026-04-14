
#include <fstream>
#include <iostream>
#include "CExperimentLogger.h"
#include "../../method/AMethod.h"
#include <string>
#include <algorithm>
#include <filesystem>
#include <numeric>
#include <sstream>
#include "../../method/methods/SO/GPHH/SGPHHLogConfig.h"
#include "../../method/methods/SO/GPHH/individual/CGPHHIndividual.h"

char* CExperimentLogger::m_OutputDirPath = nullptr;
std::vector<std::string> CExperimentLogger::m_Data;
std::string CExperimentLogger::m_OutputDataPathPrefix;
int CExperimentLogger::m_LastProgressLogged;
size_t CExperimentLogger::m_BufferSize = 10000;
bool CExperimentLogger::m_GPHHHeaderLogged = false;
bool CExperimentLogger::m_GPHHIndividualHeaderLogged = false;
bool CExperimentLogger::m_OverrideFile = false;
void CExperimentLogger::CreateOutputDataPrefix() {
    // Create the base output directory if it doesn't exist
    std::filesystem::path baseDirPath(m_OutputDirPath);
    if (!std::filesystem::exists(baseDirPath)) {
        std::filesystem::create_directories(baseDirPath);
        std::cout << "Base directory created: " << baseDirPath << std::endl;
    }

    // Create the run-specific directory
    std::filesystem::path runDirPath = baseDirPath / ("run_" + std::to_string(AMethod::m_ExperimentRunCounter));
    if (!std::filesystem::exists(runDirPath)) {
        std::filesystem::create_directories(runDirPath);
        std::cout << "Run directory created: " << runDirPath << std::endl;
    }

    m_OutputDataPathPrefix = runDirPath.string();

    std::ifstream inFile(runDirPath.string() + "/results.csv");
    if (inFile) {
        if (m_OverrideFile)
        {
            throw std::runtime_error("Results file already exists: " + runDirPath.string() + "/results.csv, no experiment files created or overwritten");

        }
        else
        {

        }
    }
}

void CExperimentLogger::AddLine(const char* line)
{
    m_Data.emplace_back(line);
    if (m_Data.size() >= m_BufferSize)
    {
        LogData();
    }
}

void CExperimentLogger::LogData()
{
    std::ofstream outFile;
    std::string outputDataPath = m_OutputDataPathPrefix + "/data.csv";
    outFile.open(outputDataPath, std::ofstream::out | std::ofstream::app); // Open in append mode
    if (!outFile.is_open())
    {
        std::cerr << "Unable to open file: " << outputDataPath << std::endl;
    }

    for (const auto& line: m_Data)
    {
        outFile << line << std::endl;
    }
    outFile.close();
    m_Data.clear();
}

void CExperimentLogger::LogResult(const char* result)
{
    LogResult(result, "results.csv");
}

void CExperimentLogger::LogResult(const char* result, const char* fileName)
{
    std::ofstream outFile;
    std::filesystem::path runDirPath = std::filesystem::path(m_OutputDataPathPrefix) / fileName;
    OpenFileForWriting(runDirPath.string().c_str(), outFile);

    outFile << result;
    outFile.close();
}

void CExperimentLogger::LogProgress(const float progress)
{
    if (m_LastProgressLogged != (int)(progress * 100)) {
        std::cout << (int)(progress * 100) << std::endl;
        m_LastProgressLogged = (int)(progress * 100);
    }
}

void CExperimentLogger::OpenFileForWriting(const char* filePath, std::ofstream& outFile)
{
    std::ifstream inFile(filePath);
    outFile.open(filePath);
    if (!outFile.is_open())
    {
        throw std::runtime_error("Unable to open file: " + std::string(filePath));
    }
}

bool CExperimentLogger::WriteSchedulerToFile(const CScheduler& schedule, const AIndividual& solution)
{
    // TODO - generic logger should not contain Scheduler logic
    char archive_filename[256];
    std::string outputDataPath = m_OutputDataPathPrefix + "/best_solution.sol";
    snprintf(archive_filename, 256, "%s",outputDataPath.c_str());
    std::ofstream arch_file(archive_filename);
    
    arch_file << "Time;Resource assignments (resource ID - task ID) " << std::endl;

    std::vector<int> startTimes = std::vector<int>();
    for (CTask task : schedule.GetTasks())
    {
        int startTime = task.GetStart();
        if (!std::count(startTimes.begin(), startTimes.end(), startTime))
            startTimes.push_back(startTime);
    }

    std::sort(startTimes.begin(), startTimes.end());

    for (int startTime : startTimes)
    {
        arch_file << startTime + 1 << " ";

        int taskId = 1;

        for (CTask task : schedule.GetTasks())
        {
            if (task.GetStart() == startTime)
            {
                TResourceID resourceID = task.GetResourceID();

                arch_file << resourceID << "-" << taskId << " ";
            }

            taskId++;
        }

        arch_file << std::endl;
    }

    arch_file.close();
    return true;
}

void CExperimentLogger::LogGPHHGeneration(int generation, const std::vector<SSOIndividual*>& population, const SGPHHLogConfig& config, long long durationMs, float optimalValue)
{
    if (!config.EnableLogging) return;
    if (generation % config.LogInterval != 0) return;

    if (!m_GPHHHeaderLogged && config.EnableGenerationReport)
    {
        std::stringstream ss;
        ss << "Generation";
        if (config.LogGenerationTime) ss << ";Time(ms)";
        if (config.LogBestFitness) ss << ";BestFitness";
        if (config.LogAvgFitness) ss << ";AvgFitness";
        if (config.LogWorstFitness) ss << ";WorstFitness";
        if (config.LogStdDevFitness) ss << ";StdDevFitness";
        if (config.LogMedianFitness) ss << ";MedianFitness";
        if (config.LogVarianceFitness) ss << ";VarianceFitness";
        if (config.LogP90Fitness) ss << ";P90Fitness";
        if (config.LogP95Fitness) ss << ";P95Fitness";
        if (config.LogBestDistance) ss << ";BestDistance";
        if (config.LogOptimalValue) ss << ";OptimalValue";
        if (config.LogBestFormula) ss << ";BestFormula";
        if (config.LogBestSolution) ss << ";BestSolution";
        AddLine(ss.str().c_str());
        m_GPHHHeaderLogged = true;
    }

    std::vector<float> fitnesses;
    fitnesses.reserve(population.size());
    for (auto* ind : population) fitnesses.push_back(ind->m_Fitness);
    std::sort(fitnesses.begin(), fitnesses.end());

    float sumFitness = std::accumulate(fitnesses.begin(), fitnesses.end(), 0.0f);
    float avgFitness = sumFitness / fitnesses.size();
    float variance = 0.0f;
    for (float f : fitnesses) variance += (f - avgFitness) * (f - avgFitness);
    variance /= fitnesses.size();
    float stdDev = std::sqrt(variance);

    auto it = std::min_element(population.begin(), population.end(), [](SSOIndividual* a, SSOIndividual* b) {
        return a->m_Fitness < b->m_Fitness;
    });
    SSOIndividual* bestInd = *it;
    CGPHHIndividual* gphhBestInd = dynamic_cast<CGPHHIndividual*>(bestInd);

    if (config.EnableConsoleOutput)
    {
        std::cout << "Gen " << generation << ": Best=" << fitnesses.front() << " Avg=" << avgFitness << " Time=" << durationMs << "ms";
        if (config.LogOptimalValue && optimalValue > 0.0f) std::cout << " Optimal=" << optimalValue;
        if (config.LogBestFormula && gphhBestInd && gphhBestInd->m_Root) std::cout << " Formula=" << gphhBestInd->m_Root->ToString();
        std::cout << std::endl;
    }

    if (config.EnableGenerationReport)
    {
        std::stringstream ss;
        ss << generation;
        if (config.LogGenerationTime) ss << ";" << durationMs;
        if (config.LogBestFitness) ss << ";" << fitnesses.front();
        if (config.LogAvgFitness) ss << ";" << avgFitness;
        if (config.LogWorstFitness) ss << ";" << fitnesses.back();
        if (config.LogStdDevFitness) ss << ";" << stdDev;
        if (config.LogMedianFitness) ss << ";" << fitnesses[fitnesses.size() / 2];
        if (config.LogVarianceFitness) ss << ";" << variance;
        if (config.LogP90Fitness) ss << ";" << fitnesses[static_cast<size_t>(fitnesses.size() * 0.9)];
        if (config.LogP95Fitness) ss << ";" << fitnesses[static_cast<size_t>(fitnesses.size() * 0.95)];
        if (config.LogBestDistance) ss << ";" << bestInd->m_Evaluation[0];
        if (config.LogOptimalValue) ss << ";" << optimalValue;
        if (config.LogBestFormula) ss << ";" << (gphhBestInd && gphhBestInd->m_Root ? gphhBestInd->m_Root->ToString() : "N/A");
        if (config.LogBestSolution)
        {
            ss << ";";
            for (size_t i = 0; i < bestInd->m_Genotype.m_IntGenotype.size(); ++i)
                ss << bestInd->m_Genotype.m_IntGenotype[i] << (i < bestInd->m_Genotype.m_IntGenotype.size() - 1 ? " " : "");
        }
        AddLine(ss.str().c_str());
    }

    if (config.LogIndividualFitness)
    {
        if (!m_GPHHIndividualHeaderLogged)
        {
            std::string outputPath = m_OutputDataPathPrefix + "/individuals.csv";
            std::ofstream outFile(outputPath);
            if (outFile.is_open()) outFile << "Generation;IndividualID;Fitness;Distance;Solution" << std::endl;
            m_GPHHIndividualHeaderLogged = true;
        }
        std::string outputPath = m_OutputDataPathPrefix + "/individuals.csv";
        std::ofstream outFile(outputPath, std::ios::app);
        if (outFile.is_open())
        {
            std::stringstream ss;
            for (size_t i = 0; i < population.size(); ++i)
            {
                ss.str(""); ss.clear();
                ss << generation << ";" << i << ";" << population[i]->m_Fitness << ";" << population[i]->m_Evaluation[0] << ";";
                for (size_t j = 0; j < population[i]->m_Genotype.m_IntGenotype.size(); ++j)
                    ss << population[i]->m_Genotype.m_IntGenotype[j] << (j < population[i]->m_Genotype.m_IntGenotype.size() - 1 ? " " : "");
                outFile << ss.str() << std::endl;
            }
        }
    }
}

void CExperimentLogger::LogGPHHFinalResult(const SSOIndividual* best, const SGPHHLogConfig& config)
{
    if (!config.EnableFinalReport) return;
    if (config.EnableConsoleOutput)
    {
        std::cout << "\n=== Final Report ===\nBest Fitness: " << best->m_Fitness << "\n";
        const CGPHHIndividual* gphhBestInd = dynamic_cast<const CGPHHIndividual*>(best);
        if (gphhBestInd && gphhBestInd->m_Root) std::cout << "Best Formula: " << gphhBestInd->m_Root->ToString() << "\n";
        std::cout << "Best Solution: ";
        for (int val : best->m_Genotype.m_IntGenotype) std::cout << val << " ";
        std::cout << "\n";
    }
}

void CExperimentLogger::ResetGPHHHeaderFlags()
{
    m_GPHHHeaderLogged = false;
    m_GPHHIndividualHeaderLogged = false;
}
