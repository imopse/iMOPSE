#pragma once

#include "Resource.h"
#include "Task.h"
#include <string>

class CMSRAInstance
{
public:

	void Clear();

	size_t GetStageCount() const { return m_StageCount; }
	const std::vector<STask>& GetTasks() const { return m_Tasks; }
	const std::vector<SResource>& GetResources() const { return m_Resources; }
	const std::vector<std::vector<float>>& GetProbMtx() const { return m_ProbMtx; }
	const std::vector<std::vector<std::vector<int>>>& GetResourceTaskTransition() const { return m_ResourceTaskTransition; }

	void SetStageCount(size_t stageCount) { m_StageCount = stageCount; }
	void SetTasks(const std::vector<STask>& tasks) { m_Tasks = tasks; }
	void SetResources(const std::vector<SResource>& resources) { m_Resources = resources; }
	void SetProbMtx(const std::vector<std::vector<float>>& probMtx) { m_ProbMtx = probMtx; }
	void SetResourceTaskTransition(std::vector<std::vector<std::vector<int>>>& resourceTaskTransition) { m_ResourceTaskTransition = resourceTaskTransition; }

	std::string m_ProblemName;

private:


	size_t m_StageCount = 0;
	std::vector<STask> m_Tasks;
	std::vector<SResource> m_Resources;
	std::vector<std::vector<float>> m_ProbMtx;
	std::vector<std::vector<std::vector<int>>> m_ResourceTaskTransition;

};