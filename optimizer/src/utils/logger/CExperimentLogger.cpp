
#include <fstream>
#include <iostream>
#include "CExperimentLogger.h"
#include "../../method/AMethod.h"
#include <string>
#include <algorithm>
#include <filesystem>

char* CExperimentLogger::m_OutputDirPath = nullptr;
std::vector<std::string> CExperimentLogger::m_Data;
std::string CExperimentLogger::m_OutputDataPathPrefix;
int CExperimentLogger::m_LastProgressLogged;
size_t CExperimentLogger::m_BufferSize = 10000;

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
        throw std::runtime_error("Results file already exists: " + runDirPath.string() + "/results.csv, no experiment files created or overwritten");
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

void CExperimentLogger::LogData(const std::string& fileName)
{
    std::ofstream outFile;
    std::string outputDataPath = m_OutputDataPathPrefix + "/" + fileName;
    outFile.open(outputDataPath, std::ofstream::out | std::ofstream::app); // Open in append mode
    if (!outFile.is_open())
    {
        std::cerr << "Unable to open file: " << outputDataPath << std::endl;
    }

    for (const auto& line : m_Data)
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
    snprintf(archive_filename, 256, outputDataPath.c_str());
    std::ofstream arch_file(archive_filename);

    arch_file << "Instance name;Duration;Cost;AvgCashFlowDev;AvgSkillOverUse;AvgUseOfResTime " << std::endl;
    
    arch_file << schedule.GetInstanceName() << ';' << solution.m_Evaluation[0] << ';' << solution.m_Evaluation[1] << ';' << solution.m_Evaluation[2] << ';' << solution.m_Evaluation[3] << ';' << solution.m_Evaluation[4] << ';' << std::endl;

    arch_file << "Hour;Resource assignments (resource ID - task ID - Duration-predecessors) " << std::endl;

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
        arch_file << startTime + 1;
        arch_file << ";";

        int taskId = 1;

        for (CTask task : schedule.GetTasks())
        {
            if (task.GetStart() == startTime)
            {
                TResourceID resourceID = task.GetResourceID();
                TTime duration = task.GetDuration();

                arch_file << resourceID << "-" << taskId << "-" << duration << "-";
                
                for (TTaskID id: task.GetPredecessors()) {
                    arch_file << id << ",";
                }

                arch_file << ";";
                    
            }

            taskId++;
        }

        arch_file << std::endl;
    }

    arch_file.close();
    return true;
}
