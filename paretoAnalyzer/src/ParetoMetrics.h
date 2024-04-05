#pragma once
#include "ParetoFront.h"

struct ParetoMetric
{
	std::string metricName;
	float metricValue = 0.f;

	ParetoMetric(const std::string& name, const float& value);
};

struct ParetoMetrics
{
	std::vector<ParetoMetric> metrics;
// 	float GD = 0.f;
// 	float IGD = 0.f;
// 	float IGDArticle = 0.f;
// 	float IGDPlus = 0.f;
// 	float PFS = 0.f;
	// Spacing, Sparsity, ...

	void SetMetric(const std::string& metricName, float metricValue);
	const ParetoMetric* GetMetric(const std::string& metricName) const;
	std::string ToString() const;
	static ParetoMetrics GetAverageMetrics(const std::vector<ParetoMetrics>& paretoMetricsToAverage);
};

class ParetoMatricsEvaluator
{
public:
	// Evaluate ParetoFront using TrueParetoFront (or it's approximation)
	ParetoMetrics EvaluateParetoFront(const ParetoFront& ParetoToEvaluate, const ParetoFront& TruePareto) const;
	ParetoMetrics EvaluateParetoFront_IGD(const ParetoFront& ParetoToEvaluate, const ParetoFront& TruePareto) const;

private:

	float CalcHV(const ParetoFront& paretoFrontToEvaluate, const std::vector<float>& refPoint) const;
	float CalcGenerationalDistance(const ParetoFront& referenceParetoFront, const ParetoFront& paretoFrontToEvaluate) const;
	float CalcGenerationalDistancePlus(const ParetoFront& referenceParetoFront, const ParetoFront& paretoFrontToEvaluate) const;
	float CalcGenerationalDistanceNonEuclidean(const ParetoFront& referenceParetoFront, const ParetoFront& paretoFrontToEvaluate) const;
};