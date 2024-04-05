#include "ParetoFront.h"
#include "ParetoMetrics.h"
#include <cmath>
#include <algorithm>

static constexpr float EPS_ACCURACY = 0.000001f;

ParetoFront ParetoFront::Merge(const ParetoFront& otherParetoFront)
{
	// We assume this and other front are valid (no duplicated or dominated solutions)

	ParetoFront mergedPareto;
	const auto& otherSolutions = otherParetoFront.solutions;
	auto& newSolutions = mergedPareto.solutions;
	// Reserve more just to make sure they all fit
	newSolutions.reserve(solutions.size() + otherSolutions.size());

	// First, add our solutions which are not dominated, we assume there are no duplicates among them
	for (size_t i = 0; i < solutions.size(); ++i)
	{
		size_t j = 0;
		bool isDominated = false;
		while (!isDominated && j < otherParetoFront.solutions.size())
		{
			isDominated = IsDominatedBy(solutions[i], otherSolutions[j]);
			++j;
		}
		if (!isDominated)
		{
			newSolutions.push_back(solutions[i]);
		}
	}

	// Now, add other solutions which are not dominated, we assume there are no duplicates among them but have to check with those we already added
	// Check against non-dominated solutions found so far
	size_t newSolutionsFromFirstPareto = newSolutions.size();
	for (size_t i = 0; i < otherSolutions.size(); ++i)
	{
		size_t j = 0;
		bool isDominatedOrDup = false;
		while (!isDominatedOrDup && j < newSolutionsFromFirstPareto)
		{
			isDominatedOrDup = IsDominatedByOrDuplicate(otherSolutions[i], newSolutions[j]);
			++j;
		}
		if (!isDominatedOrDup)
		{
			newSolutions.push_back(otherSolutions[i]);
		}
	}

	return mergedPareto;
}

size_t ParetoFront::GetNumberOfNonDominatedBy(const ParetoFront& otherParetoFront) const
{
	size_t nonDominated = 0;
    auto otherSolutions = otherParetoFront.solutions;
    otherSolutions.erase(std::remove_if(otherSolutions.begin(), otherSolutions.end(),
                                       [](const std::vector<float>& sol) { return sol.empty(); }), otherSolutions.end());
	for (size_t i = 0; i < solutions.size(); ++i)
	{
		size_t j = 0;
		bool isDominated = false;
		while (!isDominated && j < otherSolutions.size())
		{
			isDominated = IsDominatedBy(solutions[i], otherSolutions[j]);
			++j;
		}
		if (!isDominated)
		{
			++nonDominated;
		}
	}
	return nonDominated;
}

bool ParetoFront::NormalizeByMinMax(const std::vector<float>& minValues, const std::vector<float>& maxValues)
{
	if (solutions.size() <= 0)
	{
		return true;
	}

	size_t minValSize = minValues.size();
	size_t maxValSize = maxValues.size();
	size_t sol1Size = solutions[0].size();

	if (minValSize != maxValSize || minValSize != sol1Size)
	{
		return false;
	}

	std::vector<float> diffVec(sol1Size, 0.f);
	for (size_t v = 0; v < sol1Size; ++v)
	{
		diffVec[v] = maxValues[v] - minValues[v];
	}

	for (size_t i = 0; i < solutions.size(); ++i)
	{
		std::vector<float>& sol = solutions[i];
		for (size_t v = 0; v < sol.size(); ++v)
		{
			sol[v] = (sol[v] - minValues[v]) / diffVec[v];
		}
	}

	return true;
}

bool IsDominatedBy(const std::vector<float>& sol1, const std::vector<float>& sol2)
{
	size_t sol1Size = sol1.size();

	// If this has any better value
	for (int i = 0; i < 2; ++i)
	{
		if (sol1[i] + EPS_ACCURACY < sol2[i])
		{
			return false;
		}
	}

	// Now we are sure we have worse or equal values

	// If other has at least one better value
	for (int i = 0; i < 2; ++i)
	{
		if (sol2[i] + EPS_ACCURACY < sol1[i])
		{
			return true;
		}
	}

	// If equal
	return false;
}

bool IsDominatedByOrDuplicate(const std::vector<float>& sol1, const std::vector<float>& sol2)
{
	size_t sol1Size = sol1.size();

	// If this has any better value
	for (int i = 0; i < sol1Size; ++i)
	{
		if (sol1[i] + EPS_ACCURACY < sol2[i])
		{
			return false;
		}
	}

	// Now we are sure we have worse or equal values

	// If other has at least one better value
	for (int i = 0; i < sol1Size; ++i)
	{
		if (sol2[i] + EPS_ACCURACY < sol1[i])
		{
			return true;
		}
	}

	// If equal - treat duplicate as dominated
	return true;
}

bool IsDuplicate(const std::vector<float>& sol1, const std::vector<float>& sol2)
{
	size_t sol1Size = sol1.size();
	if (sol2.size() != sol1Size)
	{
		return false;
	}

	for (size_t i = 0; i < sol1Size; ++i)
	{
		if (fabsf(sol1[i] - sol2[i]) > EPS_ACCURACY)
		{
			return false;
		}
	}

	return true;
}
