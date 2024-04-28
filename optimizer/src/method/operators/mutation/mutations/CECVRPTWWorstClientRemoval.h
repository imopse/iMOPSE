#pragma once

#include <algorithm>
#include <tuple>
#include "../../../../utils/random/CRandom.h"
#include "../AMutation.h"
#include "../../../../problem/problems/ECVRPTW/CECVRPTW.h"

class CECVRPTWWorstClientRemoval : public AMutation
{
public:
	explicit CECVRPTWWorstClientRemoval(AProblem& problem) : m_problem((CECVRPTW&)problem) {};
	~CECVRPTWWorstClientRemoval() override = default;

	void Mutate(SProblemEncoding& problemEncoding, AIndividual& child) override;
private:
	CECVRPTW& m_problem;
};