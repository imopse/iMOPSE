#pragma once
#include <vector>
#include <map>
#include <string>

struct ParetoFront
{
	std::vector<std::vector<float>> solutions;

	ParetoFront Merge(const ParetoFront& otherParetoFront);
	size_t GetNumberOfNonDominatedBy(const ParetoFront& otherParetoFront) const;
	bool NormalizeByMinMax(const std::vector<float>& minValues, const std::vector<float>& maxValues);
};

bool IsDominatedBy(const std::vector<float>& sol1, const std::vector<float>& sol2);
bool IsDominatedByOrDuplicate(const std::vector<float>& sol1, const std::vector<float>& sol2);
bool IsDuplicate(const std::vector<float>& sol1, const std::vector<float>& sol2);