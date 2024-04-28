#pragma once

#include <algorithm>
#include "../../../../utils/random/CRandom.h"
#include "../AMutation.h"
#include "../../../../problem/problems/ECVRPTW/CECVRPTW.h"

class CECVRPTWRandomClientRemoval : public AMutation
{
public:
	explicit CECVRPTWRandomClientRemoval(AProblem& problem) : m_problem((CECVRPTW&)problem) {};
	~CECVRPTWRandomClientRemoval() override = default;

	void Mutate(SProblemEncoding& problemEncoding, AIndividual& child) override;

private: 
	CECVRPTW& m_problem;
};