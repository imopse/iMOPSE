#pragma once

#include <algorithm>
#include <chrono>
#include "../../../../utils/random/CRandom.h"
#include "../AMutation.h"
#include "../../../../problem/problems/ECVRPTW/CECVRPTW.h"

#define GREEDYSIZE 20

class CECVRPTWGreedyClientInsertion : public AMutation
{
public:
	explicit CECVRPTWGreedyClientInsertion(AProblem& problem, size_t objectiveIndex) : m_problem((CECVRPTW&)problem), m_objectiveIndex(objectiveIndex) {};
	~CECVRPTWGreedyClientInsertion() override = default;

	void Mutate(SProblemEncoding& problemEncoding, AIndividual& child) override;
private:
	CECVRPTW& m_problem;
	size_t m_objectiveIndex = 0;
};