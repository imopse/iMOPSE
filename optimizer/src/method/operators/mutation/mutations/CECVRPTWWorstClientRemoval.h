#pragma once

#include <algorithm>
#include <tuple>
#include "../../../../utils/random/CRandom.h"
#include "../AMutation.h"
#include "../../../../problem/problems/ECVRPTW/CECVRPTW.h"

class CECVRPTWWorstClientRemoval : public AMutation
{
public:
	explicit CECVRPTWWorstClientRemoval(AProblem& problem, size_t objectiveIndex) : m_problem((CECVRPTW&)problem), m_objectiveIndex(objectiveIndex) 
	{
		m_calculatedRemovals = new std::vector<std::tuple<size_t, float>>();
		m_calculatedRemovals->reserve(m_problem.GetProblemEncoding().m_Encoding[0].m_SectionDescription.size() + m_problem.GetECVRPTWTemplate().GetVehicleCount() - 1);
	};
	~CECVRPTWWorstClientRemoval() override
	{
		delete m_calculatedRemovals;
	};

	void Mutate(SProblemEncoding& problemEncoding, AIndividual& child) override;
private:
	CECVRPTW& m_problem;
	size_t m_objectiveIndex = 0;
	std::vector<std::tuple<size_t, float>>* m_calculatedRemovals;
};