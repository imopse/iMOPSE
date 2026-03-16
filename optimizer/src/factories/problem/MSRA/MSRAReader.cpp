#include "MSRAReader.h"
#include <regex>
#include "utils/fileReader/CReadUtils.h"

const std::string CMSRAReader::s_Delimiter = " ";
const std::string CMSRAReader::s_InstanceNameKey = "InstanceName:";
const std::string CMSRAReader::s_TaskCountKey = "TaskCount:";
const std::string CMSRAReader::s_ResourceCountKey = "ResourceCount:";
const std::string CMSRAReader::s_StageCountKey = "StageCount:";
const std::string CMSRAReader::s_TasksSectionKey = "Tasks";
const std::string CMSRAReader::s_ResourcesSectionKey = "Resources";
const std::string CMSRAReader::s_ProbMtxSectionKey = "ProbabilityMtx";
const std::string CMSRAReader::s_ResourceTaskTransitionSectionKey = "ResourceTaskTransition";

CMSRAProblem* CMSRAReader::CreateMSRA(const char* problemDefinitionPath)
{
    CMSRAInstance problemTemplate;
    CMSRAReader::ReadDefinition(problemDefinitionPath, problemTemplate);
    return new CMSRAProblem(problemTemplate);
}

bool CMSRAReader::ReadDefinition(const char* filePath, CMSRAInstance& problemTemplate)
{
	std::ifstream readFileStream(filePath);

	std::string instanceName;
	CReadUtils::GotoReadStringByKey(readFileStream, s_InstanceNameKey, s_Delimiter, instanceName);

	int taskCount = 0;
	CReadUtils::GotoReadIntegerByKey(readFileStream, s_TaskCountKey, s_Delimiter, taskCount);

	int resourceCount = 0;
	CReadUtils::GotoReadIntegerByKey(readFileStream, s_ResourceCountKey, s_Delimiter, resourceCount);

	int stageCount = 0;
	CReadUtils::GotoReadIntegerByKey(readFileStream, s_StageCountKey, s_Delimiter, stageCount);

	std::vector<STask> tasks;
	ReadTasks(readFileStream, taskCount, tasks);
	std::vector<SResource> resources;
	ReadResources(readFileStream, resourceCount, resources);
	std::vector<std::vector<float>> probMtx;
	ReadProbabilityMtx(readFileStream, resourceCount, taskCount, probMtx);
	std::vector<std::vector<std::vector<int>>> resourceTaskTransition;
	ReadResourceTaskTransition(readFileStream, resourceCount, taskCount, resourceTaskTransition);

	readFileStream.close();

	problemTemplate.m_ProblemName = instanceName.c_str();
	problemTemplate.SetStageCount(stageCount);
	problemTemplate.SetTasks(tasks);
	problemTemplate.SetResources(resources);
	problemTemplate.SetProbMtx(probMtx);
	problemTemplate.SetResourceTaskTransition(resourceTaskTransition);

	return true;
}

void CMSRAReader::ReadTasks(std::ifstream& fileStream, int taskCount, std::vector<STask>& tasks)
{
	std::string line;
	if (CReadUtils::GotoLineByKey(fileStream, s_TasksSectionKey, line))
	{
		// Header
		std::getline(fileStream, line);
        tasks.reserve(taskCount);

		for (int i = 0; i < taskCount; ++i)
		{
			if (std::getline(fileStream, line))
			{
				auto const re = std::regex{ R"(\s+)" };
				auto const vec = std::vector<std::string>(std::sregex_token_iterator{ line.begin(), line.end(), re, -1 }, std::sregex_token_iterator{});
                tasks.push_back(STask{ std::stoull(vec[0]), std::stoi(vec[1]) });
			}
		}
	}
}

void CMSRAReader::ReadResources(std::ifstream& fileStream, int resourceCount, std::vector<SResource>& resources)
{
	std::string line;
	if (CReadUtils::GotoLineByKey(fileStream, s_ResourcesSectionKey, line))
	{
		// Header
		std::getline(fileStream, line);
        resources.reserve(resourceCount);

		for (int i = 0; i < resourceCount; ++i)
		{
			if (std::getline(fileStream, line))
			{
				auto const re = std::regex{ R"(\s+)" };
				auto const vec = std::vector<std::string>(std::sregex_token_iterator{ line.begin(), line.end(), re, -1 }, std::sregex_token_iterator{});
                resources.push_back(SResource{ std::stoull(vec[0]), std::stoi(vec[1]), std::stoi(vec[2]) });
			}
		}
	}
}

void CMSRAReader::ReadProbabilityMtx(std::ifstream& fileStream, int resourceCount, int taskCount, std::vector<std::vector<float>>& probMtx)
{
	std::string line;
	if (CReadUtils::GotoLineByKey(fileStream, s_ProbMtxSectionKey, line))
	{
		// Header
		std::getline(fileStream, line);
		probMtx.reserve(resourceCount);

		// TODO - it can be improved to be error prone, for now we expect the exact order of the indices
		for (int i = 0; i < resourceCount; ++i)
		{
			probMtx.emplace_back();
			auto& resourceRow = probMtx.back();
            resourceRow.reserve(taskCount);

			for (int j = 0; j < taskCount; ++j)
			{
				if (std::getline(fileStream, line))
				{
					auto const re = std::regex{ R"(\s+)" };
					auto const vec = std::vector<std::string>(std::sregex_token_iterator{ line.begin(), line.end(), re, -1 }, std::sregex_token_iterator{});
                    resourceRow.push_back(std::stof(vec[2]));
				}
			}
		}
	}
}

void CMSRAReader::ReadResourceTaskTransition(std::ifstream& fileStream, int resourceCount, int taskCount, std::vector<std::vector<std::vector<int>>>& resourceTaskTransition)
{
	std::string line;
	// +1 task -> extra task as 'position zero' which can be used or not
	int taskPositions = taskCount + 1;
	if (CReadUtils::GotoLineByKey(fileStream, s_ResourceTaskTransitionSectionKey, line))
	{
		// Header
		std::getline(fileStream, line);
		resourceTaskTransition.reserve(resourceCount);

		// TODO - it can be improved to be error prone, for now we expect the exact order of the indices
		for (int i = 0; i < resourceCount; ++i)
		{
            resourceTaskTransition.emplace_back();
			auto& resourceSection = resourceTaskTransition.back();
            resourceSection.reserve(taskPositions);

			for (int j = 0; j < taskPositions; ++j)
			{
                resourceSection.emplace_back();
				auto& fromTask = resourceSection.back();
                fromTask.reserve(taskPositions);

				for (int k = 0; k < taskPositions; ++k)
				{
					if (std::getline(fileStream, line))
					{
						auto const re = std::regex{ R"(\s+)" };
						auto const vec = std::vector<std::string>(std::sregex_token_iterator{ line.begin(), line.end(), re, -1 }, std::sregex_token_iterator{});
                        fromTask.push_back(std::stoi(vec[3]));
					}
				}
			}
		}
	}
}
