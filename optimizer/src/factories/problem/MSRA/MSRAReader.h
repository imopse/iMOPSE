#pragma once

#include "problem/problems/MSRA/MSRAProblem.h"
#include <string>
#include <fstream>

class CMSRAReader
{
public:
    static CMSRAProblem* CreateMSRA(const char* problemDefinitionPath);

private:
    static bool ReadDefinition(const char* filePath, CMSRAInstance& problemTemplate);
    static void ReadTasks(std::ifstream& fileStream, int taskCount, std::vector<STask>& tasks);
    static void ReadResources(std::ifstream& fileStream, int resourceCount, std::vector<SResource>& resources);
    static void ReadProbabilityMtx(std::ifstream& fileStream, int resourceCount, int taskCount, std::vector<std::vector<float>>& probMtx);
    static void ReadResourceTaskTransition(std::ifstream& fileStream, int resourceCount, int taskCount, std::vector<std::vector<std::vector<int>>>& resourceTaskTransition);

	static const std::string s_Delimiter;
	static const std::string s_InstanceNameKey;
	static const std::string s_TaskCountKey;
	static const std::string s_ResourceCountKey;
	static const std::string s_StageCountKey;
	static const std::string s_TasksSectionKey;
	static const std::string s_ResourcesSectionKey;
	static const std::string s_ProbMtxSectionKey;
	static const std::string s_ResourceTaskTransitionSectionKey;
};
