
#include "utils/fileReader/CReadUtils.h"
#include <fstream>
#include "CMSRCPSP_Factory.h"

const std::string CMSRCPSP_Factory::s_Delimiter = " ";
const std::string CMSRCPSP_Factory::s_TasksKey = "Tasks:";
const std::string CMSRCPSP_Factory::s_ResourcesKey = "Resources:";
const std::string CMSRCPSP_Factory::s_ResourcesSectionKey = "ResourceID";
const std::string CMSRCPSP_Factory::s_TasksSectionKey = "TaskID";

CScheduler *CMSRCPSP_Factory::scheduler = nullptr;

CMSRCPSP_TA *CMSRCPSP_Factory::CreateMSRCPSP_TA(const char *problemConfigurationPath, size_t objCount)
{
    scheduler = CreateScheduler(problemConfigurationPath);

    return new CMSRCPSP_TA(*scheduler, objCount);
}

CMSRCPSP_TO* CMSRCPSP_Factory::CreateMSRCPSP_TO(const char* problemConfigurationPath, size_t objCount)
{
    scheduler = CreateScheduler(problemConfigurationPath);

    return new CMSRCPSP_TO(*scheduler, objCount);
}

void CMSRCPSP_Factory::DeleteObjects()
{
    delete scheduler;
}

CScheduler *CMSRCPSP_Factory::CreateScheduler(const char *problemConfigurationPath)
{
    scheduler = new CScheduler();

    std::ifstream readFileStream(problemConfigurationPath);
    
    std::string instanceName;
    if (!CReadUtils::GotoReadStringByKey(readFileStream, "File name:", s_Delimiter, instanceName))
    {
        throw std::runtime_error("Error reading instance name for MSRCPSP");
    }
    scheduler->SetInstanceName(instanceName);
    
    int tasksNumber = 0;
    if (!CReadUtils::GotoReadIntegerByKey(readFileStream, s_TasksKey, s_Delimiter, tasksNumber))
    {
        throw std::runtime_error("Error reading task number for MSRCPSP");
    }

    int resourcesNumber = 0;
    if (!CReadUtils::GotoReadIntegerByKey(readFileStream, s_ResourcesKey, s_Delimiter, resourcesNumber))
    {
        throw std::runtime_error("Error reading resource number for MSRCPSP");
    }

    std::vector<CResource> resources;
    ReadResources(readFileStream, resourcesNumber, resources);
    std::vector<CTask> tasks;
    ReadTasks(readFileStream, tasksNumber, tasks);

    readFileStream.close();

    for (const CTask &task: tasks)
    {
        for (TTaskID predecessorTaskId: task.GetPredecessors())
        {
            tasks[(size_t) predecessorTaskId - 1].SetHasSuccessors(true);
        }
    }

    scheduler->SetResources(resources);
    scheduler->SetTasks(tasks);
    scheduler->Reset();

    return scheduler;
}

void CMSRCPSP_Factory::ReadResources(std::basic_ifstream<char>& fileStream, int resourcesNumber, std::vector<CResource>& resources)
{
    std::string line;
    if (!CReadUtils::GotoLineByKey(fileStream, s_ResourcesSectionKey, line))
    {
        throw std::runtime_error("Error reading resources for MSRCPSP");
    }

    for (int i = 0; i < resourcesNumber; ++i)
    {
        if (std::getline(fileStream, line))
        {
            auto const vec = CReadUtils::SplitLine(line);

            std::vector<SSkill> skills;
            for (size_t i = 2; i + 1 < vec.size(); i += 2)
            {
                skills.emplace_back(std::stoi(vec[i].substr(1, vec[i].length() - 1)), std::stoi(vec[i + 1]));
            }
            resources.emplace_back(std::stoi(vec[0]), std::stof(vec[1]), skills);
        }
    }
}

void CMSRCPSP_Factory::ReadTasks(std::basic_ifstream<char>& fileStream, int tasksNumber, std::vector<CTask>& tasks)
{
    std::string line;
    if (!CReadUtils::GotoLineByKey(fileStream, s_TasksSectionKey, line))
    {
        throw std::runtime_error("Error reading tasks for MSRCPSP");
    }

    for (int i = 0; i < tasksNumber; ++i)
    {
        if (std::getline(fileStream, line))
        {
            auto const vec = CReadUtils::SplitLine(line);

            size_t tokenIndex = 2;
            std::vector<SSkill> skills;
            while (tokenIndex + 1 < vec.size() && vec[tokenIndex].substr(0, 1) == "Q")
            {
                skills.emplace_back(std::stoi(vec[tokenIndex].substr(1, vec[tokenIndex].length() - 1)),
                    std::stoi(vec[tokenIndex + 1]));
                tokenIndex += 2;
            }

            std::vector<TTaskID> predecessors;
            while (tokenIndex < vec.size())
            {
                predecessors.push_back(std::stoi(vec[tokenIndex]));
                ++tokenIndex;
            }
            tasks.emplace_back(std::stoi(vec[0]), skills, std::stoi(vec[1]), predecessors);
        }
    }
}