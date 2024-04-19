#pragma once

#include <algorithm>
#include <chrono>
#include "../../../../utils/random/CRandom.h"
#include "../AMutation.h"
#include "../../../../problem/problems/ECVRPTW/CECVRPTW.h"

#define DEMANDWEIGHT 0.1
#define TIMEWINDOWWEIGHT 0.6
#define DISTANCEWEIGHT 0.3

class CECVRPTWShawClientInsertion : public AMutation
{
public:
	explicit CECVRPTWShawClientInsertion(AProblem& problem) : m_problem((CECVRPTW&)problem) {};
	~CECVRPTWShawClientInsertion() override = default;

	void Mutate(SProblemEncoding& problemEncoding, AIndividual& child) override;
private:
	CECVRPTW& m_problem;
};