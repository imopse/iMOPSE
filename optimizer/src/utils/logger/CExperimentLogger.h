#pragma once

#include <vector>
#include "../../problem/problems/MSRCPSP/CScheduler.h"
#include "../../method/individual/AIndividual.h"
#include "../../method/individual/SO/SSOIndividual.h"
#include <string>

struct SGPHHLogConfig;

class CExperimentLogger
{
public:
    static char* m_OutputDirPath;
    static std::string m_OutputDataPathPrefix;
    static bool m_OverrideFile;

    static void CreateOutputDataPrefix();
    static void AddLine(const char* line);
    static void LogData();
    static void LogResult(const char* result);
    static void LogResult(const char* result, const char* fileName);
    static void LogProgress(const float progress);
    static bool WriteSchedulerToFile(const CScheduler& schedule, const AIndividual& solution);

    // GPHH Logging
    static void LogGPHHGeneration(int generation, const std::vector<SSOIndividual*>& population, const SGPHHLogConfig& config, long long durationMs, float optimalValue);
    static void LogGPHHFinalResult(const SSOIndividual* best, const SGPHHLogConfig& config);
    static void ResetGPHHHeaderFlags();

private:
    static bool m_GPHHHeaderLogged;
    static bool m_GPHHIndividualHeaderLogged;
    static size_t m_BufferSize;
    static std::vector<std::string> m_Data;
    static int m_LastProgressLogged;
    static void OpenFileForWriting(const char* filePath, std::ofstream& outFile);
};
