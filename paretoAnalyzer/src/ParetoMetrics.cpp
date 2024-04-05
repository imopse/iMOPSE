#include "ParetoMetrics.h"
#include <algorithm>
#include <cmath>
#include <float.h>

ParetoMetric::ParetoMetric(const std::string& name, const float& value)
	: metricName(name)
	, metricValue(value)
{
}

void ParetoMetrics::SetMetric(const std::string& metricName, float metricValue)
{
	auto metricIt = std::find_if(metrics.begin(), metrics.end(), [metricName](const ParetoMetric& metric)
	{
		return metric.metricName.compare(metricName) == 0;
	});

	if (metricIt != metrics.end())
	{
		metricIt->metricValue = metricValue;
	}
	else
	{
		metrics.emplace_back(metricName, metricValue);
	}
}

const ParetoMetric* ParetoMetrics::GetMetric(const std::string& metricName) const
{
	auto metricIt = std::find_if(metrics.begin(), metrics.end(), [metricName](const ParetoMetric& metric)
	{
		return metric.metricName.compare(metricName) == 0;
	});

	if (metricIt != metrics.end())
	{
		return &(*metricIt);
	}
	else
	{
		return nullptr;
	}
}

std::string ParetoMetrics::ToString() const
{
	std::string toString = "";
	for (const ParetoMetric& paretoMetric : metrics)
	{
		char buff[100];
		snprintf(buff, sizeof(buff), "%s:%.5f;", paretoMetric.metricName.c_str(), paretoMetric.metricValue);
		toString.append(buff);
	}
	return toString;
}

ParetoMetrics ParetoMetrics::GetAverageMetrics(const std::vector<ParetoMetrics>& paretoMetricsToAverage)
{
	ParetoMetrics avgParetoMetrics;
	//std::vector<std::string> metricsToAvg{"GD", "IGD", "IGDArticle", "IGD+", "PFS"};
	//std::vector<std::string> metricsToAvg{"GD", "IGD", "PFS"};
	//std::vector<std::string> metricsToAvg{"GD", "IGD", "PFS", "ND", "ND/TPFS"};
	std::vector<std::string> metricsToAvg{"HV", "GD", "IGD", "PFS", "ND", "ND/TPFS"};
	std::string stdPrefix("_std");

	for (const std::string& metricName : metricsToAvg)
	{
		float avgValue = 0.f;
		size_t valCount = 0;
		for (size_t i = 0; i < paretoMetricsToAverage.size(); ++i)
		{
			if (const ParetoMetric* paretoMetric = paretoMetricsToAverage[i].GetMetric(metricName))
			{
				avgValue += paretoMetric->metricValue;
				++valCount;
			}
		}

		if (valCount > 0)
		{
			avgValue /= paretoMetricsToAverage.size();
			avgParetoMetrics.SetMetric(metricName, avgValue);

			// Calculate std
			float stdVal = 0.f;
			for (size_t i = 0; i < paretoMetricsToAverage.size(); ++i)
			{
				if (const ParetoMetric* paretoMetric = paretoMetricsToAverage[i].GetMetric(metricName))
				{
					stdVal += powf(paretoMetric->metricValue - avgValue, 2);
				}
			}
			avgParetoMetrics.SetMetric(metricName + stdPrefix, sqrtf(stdVal / valCount));
		}

	}

	return avgParetoMetrics;
}

ParetoMetrics ParetoMatricsEvaluator::EvaluateParetoFront(const ParetoFront& ParetoToEvaluate, const ParetoFront& TruePareto) const
{
	ParetoMetrics paretoMetrics;
	//paretoMetrics.SetMetric("IGDArticle", CalcGenerationalDistanceNonEuclidean(TruePareto, ParetoToEvaluate));
	paretoMetrics.SetMetric("HV", CalcHV(ParetoToEvaluate, {1.f, 1.f}));
	paretoMetrics.SetMetric("IGD", CalcGenerationalDistance(TruePareto, ParetoToEvaluate));
	paretoMetrics.SetMetric("GD", CalcGenerationalDistance(ParetoToEvaluate, TruePareto));
	paretoMetrics.SetMetric("PFS", (float)ParetoToEvaluate.solutions.size());
	float nonDominated = (float)ParetoToEvaluate.GetNumberOfNonDominatedBy(TruePareto);
	paretoMetrics.SetMetric("ND", nonDominated);
	paretoMetrics.SetMetric("ND/TPFS", nonDominated / (float)TruePareto.solutions.size());
	//paretoMetrics.SetMetric("IGD+", CalcGenerationalDistancePlus(TruePareto, ParetoToEvaluate));

	// TO-DO evaluate other metrics

	return paretoMetrics;
}

ParetoMetrics ParetoMatricsEvaluator::EvaluateParetoFront_IGD(const ParetoFront& ParetoToEvaluate, const ParetoFront& TruePareto) const
{
	ParetoMetrics paretoMetrics;
	paretoMetrics.SetMetric("IGD", CalcGenerationalDistance(TruePareto, ParetoToEvaluate));
	return paretoMetrics;
}

float CalcDist2(const std::vector<float>& vec1, const std::vector<float>& vec2)
{
	// We assume they are equal sizes
	size_t s = vec1.size();
	float dist2 = 0.f;
	float tempDiff = 0.f;
	for (size_t i = 0; i < s; ++i)
	{
		tempDiff = vec2[i] - vec1[i];
		dist2 += (tempDiff * tempDiff);
	}
	return dist2;
}

float CalcDist2Plus(const std::vector<float>& vec1, const std::vector<float>& vec2)
{
	// We assume they are equal sizes
	size_t s = vec1.size();
	float dist2 = 0.f;
	float tempDiff = 0.f;
	for (size_t i = 0; i < s; ++i)
	{
		tempDiff = fmaxf(0.f, vec2[i] - vec1[i]);
		dist2 += (tempDiff * tempDiff);
	}
	return dist2;
}

float CalcDistNonEuclidean(const std::vector<float>& vec1, const std::vector<float>& vec2)
{
	// We assume they are equal sizes
	size_t s = vec1.size();
	float dist = 0.f;
	for (size_t i = 0; i < s; ++i)
	{
		dist += fabsf(vec2[i] - vec1[i]);
	}
	return dist;
}

// Calculate Hyper-volume using values as they are (either absolute or normalized), using the reference point
float ParetoMatricsEvaluator::CalcHV(const ParetoFront& paretoFrontToEvaluate, const std::vector<float>& refPoint) const
{
    auto solutionsCopy = paretoFrontToEvaluate.solutions;

    solutionsCopy.erase(std::remove_if(solutionsCopy.begin(), solutionsCopy.end(),
                                       [](const std::vector<float>& sol) { return sol.empty(); }), solutionsCopy.end());
    
    auto sortLambda = [](const std::vector<float>& lhv, const std::vector<float>& rhv) -> bool
    {
        return lhv[0] < rhv[0];
    };
    
    std::sort(solutionsCopy.begin(), solutionsCopy.end(), sortLambda);

	float hyperVolume = 0.f;
	float prevCost = refPoint[1];
	for (const std::vector<float>& sol : solutionsCopy)
	{
		hyperVolume += ((refPoint[0] - sol[0]) * (prevCost - sol[1]));
		prevCost = sol[1];
	}

	return hyperVolume;
}

// By a common distance calculation
float ParetoMatricsEvaluator::CalcGenerationalDistance(const ParetoFront& referenceParetoFront, const ParetoFront& paretoFrontToEvaluate) const
{
	float distSum = 0.f;
	const auto& referenceSolutions = referenceParetoFront.solutions;
	auto otherSolutions = paretoFrontToEvaluate.solutions;
    otherSolutions.erase(std::remove_if(otherSolutions.begin(), otherSolutions.end(),
                                       [](const std::vector<float>& sol) { return sol.empty(); }), otherSolutions.end());
	const size_t otherSolSize = otherSolutions.size();
	float minDist2 = FLT_MAX;
	float tempDist2 = FLT_MAX;

	for (size_t i = 0; i < referenceSolutions.size(); ++i)
	{
		// Find closest point to true point
		minDist2 = FLT_MAX;
		for (size_t j = 0; j < otherSolSize; ++j)
		{
			tempDist2 = CalcDist2(referenceSolutions[i], otherSolutions[j]);
			if (tempDist2 < minDist2)
			{
				minDist2 = tempDist2;
			}
		}

		// Do not calculate sqrtf like normally, use dist2 and normalize at the end -> as described in equation
		//distSum += sqrtf(minDist2);
		distSum += minDist2;
	}

	return sqrtf(distSum) / referenceSolutions.size();
}

float ParetoMatricsEvaluator::CalcGenerationalDistancePlus(const ParetoFront& referenceParetoFront, const ParetoFront& paretoFrontToEvaluate) const
{
	float distSum = 0.f;
	const auto& referenceSolutions = referenceParetoFront.solutions;
	const auto& otherSolutions = paretoFrontToEvaluate.solutions;
	const size_t otherSolSize = otherSolutions.size();
	float minDist2 = FLT_MAX;
	float tempDist2 = FLT_MAX;

	for (size_t i = 0; i < referenceSolutions.size(); ++i)
	{
		// Find closest point to true point
		minDist2 = FLT_MAX;
		for (size_t j = 0; j < otherSolSize; ++j)
		{
			tempDist2 = CalcDist2Plus(referenceSolutions[i], otherSolutions[j]);
			if (tempDist2 < minDist2)
			{
				minDist2 = tempDist2;
			}
		}

		// Do not calculate sqrtf like normally, use dist2 -> as described in equation
		distSum += minDist2;
	}

	// Do not calculate sqrt as in the equation
	return distSum / referenceSolutions.size();
}

// By a distance calculation described in the article
float ParetoMatricsEvaluator::CalcGenerationalDistanceNonEuclidean(const ParetoFront& referenceParetoFront, const ParetoFront& paretoFrontToEvaluate) const
{
	float distSum = 0.f;
	const auto& referenceSolutions = referenceParetoFront.solutions;
	const auto& otherSolutions = paretoFrontToEvaluate.solutions;
	const size_t otherSolSize = otherSolutions.size();
	float minDist = FLT_MAX;
	float tempDist = FLT_MAX;

	for (size_t i = 0; i < referenceSolutions.size(); ++i)
	{
		// Find closest point to true point
		minDist = FLT_MAX;
		for (size_t j = 0; j < otherSolSize; ++j)
		{
			tempDist = CalcDistNonEuclidean(referenceSolutions[i], otherSolutions[j]);
			if (tempDist < minDist)
			{
				minDist = tempDist;
			}
		}

		distSum += (minDist * minDist);
	}

	return sqrtf(distSum) / referenceSolutions.size();
}
