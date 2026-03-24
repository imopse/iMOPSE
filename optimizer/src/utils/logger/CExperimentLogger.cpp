
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
    // Nazwa pliku unikalna po instancji + makespan + cost
    const int makespan = solution.m_Evaluation.size() > 0 ? (int)solution.m_Evaluation[0] : -1;
    const int cost = solution.m_Evaluation.size() > 1 ? (int)solution.m_Evaluation[1] : -1;

    std::string instanceName = schedule.GetInstanceName();
    for (char& c : instanceName)
    {
        if (c == '/' || c == '\\' || c == ' ' || c == ';' || c == ':')
            c = '_';
    }

    std::string outputDataPath =
        m_OutputDataPathPrefix + "/sol_" + instanceName +
        "_m" + std::to_string(makespan) +
        "_c" + std::to_string(cost) + ".sol";

    std::ofstream arch_file(outputDataPath);
    if (!arch_file.is_open())
    {
        std::cerr << "Unable to open file: " << outputDataPath << std::endl;
        return false;
    }

    // Linia meta - skrypt j¹ zignoruje, bo nie zaczyna siê od inta
    arch_file << "Meta Instance=" << schedule.GetInstanceName()
        << " Makespan=" << makespan
        << " Cost=" << cost << std::endl;

    // Linia nag³ówka - te¿ bêdzie zignorowana przez parser
    arch_file << "Hour Resource-Task" << std::endl;

    std::vector<int> startTimes;
    for (const CTask& task : schedule.GetTasks())
    {
        int startTime = task.GetStart();
        if (std::find(startTimes.begin(), startTimes.end(), startTime) == startTimes.end())
            startTimes.push_back(startTime);
    }

    std::sort(startTimes.begin(), startTimes.end());

    for (int startTime : startTimes)
    {
        arch_file << (startTime + 1);

        for (const CTask& task : schedule.GetTasks())
        {
            if (task.GetStart() == startTime)
            {
                TResourceID resourceID = task.GetResourceID();
                TTaskID taskID = task.GetTaskID();   // wa¿ne: bierzemy prawdziwe ID zadania

                arch_file << " " << resourceID << "-" << taskID;
            }
        }

        arch_file << std::endl;
    }

    arch_file.close();
    return true;
}
