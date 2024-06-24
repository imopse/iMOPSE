#include "ConfigResults.h"

bool ConfigResults::Contains(const std::string& instanceName) const
{
	return m_ConfigParetos.count(instanceName) > 0;
}

void ConfigResults::AddParetoFront(const std::string& instanceName, const ParetoFront& newPareto)
{
	if (!Contains(instanceName))
	{
		m_ConfigParetos[instanceName] = std::vector<ParetoFront>();
	}

	m_ConfigParetos[instanceName].push_back(newPareto);
}

size_t ConfigResults::GetParetoCountForInstance(const std::string& instanceName)
{
	if (Contains(instanceName))
	{
		return (m_ConfigParetos[instanceName]).size();
	}
	else
	{
		return 0;
	}
}

ParetoFront ConfigResults::GetMergedParetoFronts(const std::string& instanceName)
{
	ParetoFront newParetoFront;

	if (Contains(instanceName))
	{
		const std::vector<ParetoFront>& paretoFronts = m_ConfigParetos[instanceName];
		const size_t frontsSize = paretoFronts.size();

		if (frontsSize > 0)
		{
			newParetoFront = paretoFronts[0];
		}

		for (size_t i = 1; i < frontsSize; ++i)
		{
			newParetoFront = newParetoFront.Merge(paretoFronts[i]);
		}
	}

	return newParetoFront;
}

ParetoMetrics ConfigResults::EvaluateByTrueParetoFront(const std::string& instanceName, const ParetoFront& trueParetoFront)
{
	const ParetoMatricsEvaluator paretoEvaluator;
	std::vector<ParetoMetrics> allParetoMetrics;

	if (Contains(instanceName))
	{
		const std::vector<ParetoFront>& paretoFronts = m_ConfigParetos[instanceName];
		const size_t frontsSize = paretoFronts.size();

		for (size_t i = 0; i < frontsSize; ++i)
		{
			if (paretoFronts[i].solutions.size() > 0) {
				ParetoMetrics paretoMetrics = paretoEvaluator.EvaluateParetoFront(paretoFronts[i], trueParetoFront);
				allParetoMetrics.push_back(paretoMetrics);
			}
			else {
				ParetoMetrics paretoMetrics;
				paretoMetrics.SetMetric("HV", 0);
				paretoMetrics.SetMetric("IGD", 1);
				paretoMetrics.SetMetric("GD", 1);
				paretoMetrics.SetMetric("PFS", 0);
				paretoMetrics.SetMetric("ND", 0);
				paretoMetrics.SetMetric("ND/TPFS", 0);
				allParetoMetrics.push_back(paretoMetrics);
			}
			// Temp
			//std::cout << paretoMetrics.ToString() << std::endl;
		}
	}

	return ParetoMetrics::GetAverageMetrics(allParetoMetrics);
}

ParetoMetrics ConfigResults::EvaluateByTrueParetoFront_IGD(const std::string& instanceName, const ParetoFront& trueParetoFront)
{
	const ParetoMatricsEvaluator paretoEvaluator;
	std::vector<ParetoMetrics> allParetoMetrics;

	if (Contains(instanceName))
	{
		const std::vector<ParetoFront>& paretoFronts = m_ConfigParetos[instanceName];
		const size_t frontsSize = paretoFronts.size();

		for (size_t i = 0; i < frontsSize; ++i)
		{
			ParetoMetrics paretoMetrics = paretoEvaluator.EvaluateParetoFront_IGD(paretoFronts[i], trueParetoFront);
			allParetoMetrics.push_back(paretoMetrics);
		}
	}

	return ParetoMetrics::GetAverageMetrics(allParetoMetrics);
}

bool ConfigResults::NormalizeByMinMax(const std::string& instanceName, const std::vector<float>& minValues, const std::vector<float>& maxValues)
{
	if (Contains(instanceName))
	{
		std::vector<ParetoFront>& paretoFronts = m_ConfigParetos[instanceName];
		const size_t frontsSize = paretoFronts.size();

		for (size_t i = 0; i < frontsSize; ++i)
		{
			if (!paretoFronts[i].NormalizeByMinMax(minValues, maxValues))
			{
				return false;
			}
		}
	}
	else
	{
		return false;
	}

	return true;
}
