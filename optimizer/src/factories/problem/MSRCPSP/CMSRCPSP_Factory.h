#pragma once

#include <fstream>
#include "../../../problem/problems/MSRCPSP/CMSRCPSP_TA.h"
#include "../../../problem/problems/MSRCPSP/CMSRCPSP_TO.h"
#include "problem/problems/MSRCPSP/CMSRCPSP_TA_FLOAT.h"
#include "problem/problems/MSRCPSP/CMSRCPSP_TO_FLOAT.h"
#include <cstddef>

class CMSRCPSP_Factory
{
public:
    static CMSRCPSP_TA *CreateMSRCPSP_TA(const char *problemConfigurationPath, std::size_t objCount);
    static CMSRCPSP_TA_FLOAT *CreateMSRCPSP_TA_FLOAT(const char *problemConfigurationPath, std::size_t objCount);
    static CMSRCPSP_TO *CreateMSRCPSP_TO(const char* problemConfigurationPath, std::size_t objCount);
    static CMSRCPSP_TO_FLOAT *CreateMSRCPSP_TO_FLOAT(const char* problemConfigurationPath, std::size_t objCount);
private:
    static const std::string s_Delimiter;
    static const std::string s_TasksKey;
    static const std::string s_ResourcesKey;
    static const std::string s_ResourcesSectionKey;
    static const std::string s_TasksSectionKey;

    static CScheduler *CreateScheduler(const char *problemConfigurationPath);
    static void ReadResources(std::basic_ifstream<char> &fileStream, int resourcesNumber, std::vector<CResource> &resources);
    static void ReadTasks(std::basic_ifstream<char> &fileStream, int tasksNumber, std::vector<CTask> &tasks);
};
