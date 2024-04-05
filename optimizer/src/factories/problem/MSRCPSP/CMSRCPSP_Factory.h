#pragma once

#include <fstream>
#include "../../../problem/problems/MSRCPSP/CMSRCPSP_TA.h"
#include "../../../problem/problems/MSRCPSP/CMSRCPSP_TO_A.h"
#include "../../../problem/problems/MSRCPSP/CMSRCPSP_TO_P.h"

class CMSRCPSP_Factory
{
public:
    static CMSRCPSP_TA *CreateMSRCPSP_TA(const char *problemConfigurationPath, size_t objCount);
    static CMSRCPSP_TO_P *CreateMSRCPSP_TO_P(const char* problemConfigurationPath);
    static CMSRCPSP_TO_A* CreateMSRCPSP_TO_A(const char* problemConfigurationPath);
    static void DeleteObjects();
private:
    static const std::string s_Delimiter;
    static const std::string s_TasksKey;
    static const std::string s_ResourcesKey;
    static const std::string s_ResourcesSectionKey;
    static const std::string s_TasksSectionKey;

    static CScheduler *scheduler;

    static CScheduler *CreateScheduler(const char *problemConfigurationPath);
    static void ReadResources(std::basic_ifstream<char> &fileStream, int resourcesNumber, std::vector<CResource> &resources);
    static void ReadTasks(std::basic_ifstream<char> &fileStream, int tasksNumber, std::vector<CTask> &tasks);
};