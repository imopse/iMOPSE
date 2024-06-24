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
	explicit CECVRPTWShawClientInsertion(AProblem& problem) 
		: m_problem((CECVRPTW&)problem) 
	{
		m_genotypeCopy = new std::vector<int>();
		m_missingCustomers = new std::vector<int>();
		m_genotypeCopy->reserve(GENOTYPEBUFFERSIZE);
		m_missingCustomers->reserve(GENOTYPEBUFFERSIZE);
	};
	~CECVRPTWShawClientInsertion() override
	{
		delete m_genotypeCopy;
		delete m_missingCustomers;
	};

	void Mutate(SProblemEncoding& problemEncoding, AIndividual& child) override;
private:
	CECVRPTW& m_problem;
	std::vector<int>* m_genotypeCopy;
	std::vector<int>* m_missingCustomers;
};