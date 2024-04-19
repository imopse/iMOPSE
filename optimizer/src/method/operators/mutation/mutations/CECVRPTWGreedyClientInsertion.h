#pragma once

#include <algorithm>
#include <chrono>
#include "../../../../utils/random/CRandom.h"
#include "../AMutation.h"
#include "../../../../problem/problems/ECVRPTW/CECVRPTW.h"

class CECVRPTWGreedyClientInsertion : public AMutation
{
public:
	explicit CECVRPTWGreedyClientInsertion(AProblem& problem) : m_problem((CECVRPTW&)problem) {};
	~CECVRPTWGreedyClientInsertion() override = default;

	void Mutate(SProblemEncoding& problemEncoding, AIndividual& child) override;
private:
	CECVRPTW& m_problem;
};