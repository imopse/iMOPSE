#pragma once

#include <algorithm>
#include <chrono>
#include "../../../../utils/random/CRandom.h"
#include "../AMutation.h"
#include "../../../../problem/problems/ECVRPTW/CECVRPTW.h"

#define GREEDYSIZE 10

class CECVRPTWGreedyClientInsertion : public AMutation
{
public:
	explicit CECVRPTWGreedyClientInsertion(AProblem& problem, size_t objectiveIndex) 
		: m_problem((CECVRPTW&)problem)
		, m_objectiveIndex(objectiveIndex) 
	{
		m_genotypeCopy = new std::vector<int>();
		m_missingCustomers = new std::vector<int>();
		m_genotypeCopy->reserve(GENOTYPEBUFFERSIZE);
		m_missingCustomers->reserve(GENOTYPEBUFFERSIZE);
	};

	~CECVRPTWGreedyClientInsertion() override 
	{
		delete m_genotypeCopy;
		delete m_missingCustomers;
	};

	void Mutate(SProblemEncoding& problemEncoding, AIndividual& child) override;
private:
	CECVRPTW& m_problem;
	size_t m_objectiveIndex = 0;
	std::vector<int>* m_genotypeCopy;
	std::vector<int>* m_missingCustomers;
};