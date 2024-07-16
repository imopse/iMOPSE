#pragma once

#include <algorithm>
#include "../../../../utils/random/CRandom.h"
#include "../AMutation.h"
#include "../../../../problem/problems/ECVRPTW/CECVRPTW.h"

#define DEMANDWEIGHT 0.1
#define TIMEWINDOWWEIGHT 0.6
#define DISTANCEWEIGHT 0.3

class CECVRPTWShawClientRemoval : public AMutation
{
public:
	explicit CECVRPTWShawClientRemoval(AProblem& problem) : m_problem((CECVRPTW&)problem) 
	{
		m_customerIndexes = new std::vector<int>();
		m_customerIndexes->reserve(m_problem.GetProblemEncoding().m_Encoding[0].m_SectionDescription.size() - m_problem.GetECVRPTWTemplate().GetVehicleCount() - 1);
	};
	~CECVRPTWShawClientRemoval() override
	{
		delete m_customerIndexes;
	};

	void Mutate(SProblemEncoding& problemEncoding, AIndividual& child) override;
private:
	CECVRPTW& m_problem;
	std::vector<int>* m_customerIndexes;
};