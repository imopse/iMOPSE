#pragma once

#include <vector>
#include "../../problem/problems/MSRCPSP/CScheduler.h"
#include "../../method/individual/AIndividual.h"
#include <string>

class CExperimentLogger
{
public:
    static char* m_OutputDirPath;
    static std::string m_OutputDataPathPrefix;

    static void CreateOutputDataPrefix();
    static void AddLine(const char* line);
    static void LogData();
    static void LogResult(const char* result);
    static void LogResult(const char* result, const char* fileName);
    static void LogProgress(const float progress);
    static bool WriteSchedulerToFile(const CScheduler& schedule, const AIndividual& solution);
private:
    static size_t m_BufferSize;
    static std::vector<std::string> m_Data;
    static int m_LastProgressLogged;
    static void OpenFileForWriting(const char* filePath, std::ofstream& outFile);
};
