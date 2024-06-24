#pragma once
#include <utility>

#include "ParetoMetrics.h"

class ConfigResults
{
public:

	bool Contains(const std::string& instanceName) const;
	void AddParetoFront(const std::string& instanceName, const ParetoFront& newPareto);
	size_t GetParetoCountForInstance(const std::string& instanceName);
	ParetoFront GetMergedParetoFronts(const std::string& instanceName);
	ParetoMetrics EvaluateByTrueParetoFront(const std::string& instanceName, const ParetoFront& trueParetoFront);
	ParetoMetrics EvaluateByTrueParetoFront_IGD(const std::string& instanceName, const ParetoFront& trueParetoFront);
	bool NormalizeByMinMax(const std::string& instanceName, const std::vector<float>& minValues, const std::vector<float>& maxValues);
	std::map<std::string, std::vector<ParetoFront>> m_ConfigParetos;

private:

};

struct ConfigData
{
    std::string configName;
	std::string configPath;
	ConfigResults configResults;
	std::vector<std::string> configResultsDump;
	ParetoFront configMergedPareto;

	ConfigData(std::string name, std::string path)
		: configName(std::move(name))
		, configPath(std::move(path))
	{}
};