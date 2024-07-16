#pragma once

#include <algorithm>
#include <chrono>
#include "../../../../utils/random/CRandom.h"
#include "../AMutation.h"
#include "../../../../problem/problems/ECVRPTW/CECVRPTW.h"

class CECVRPTWRandomClientInsertion : public AMutation
{
public:
	explicit CECVRPTWRandomClientInsertion(AProblem& problem) : m_problem((CECVRPTW&)problem) 
	{
		m_genotypeCopy = new std::vector<int>();
		m_missingCustomers = new std::vector<int>();
		m_genotypeCopy->reserve(GENOTYPEBUFFERSIZE);
		m_missingCustomers->reserve(GENOTYPEBUFFERSIZE);
	};
	~CECVRPTWRandomClientInsertion() override = default;

	void Mutate(SProblemEncoding& problemEncoding, AIndividual& child) override;

private: 
	CECVRPTW& m_problem;
	std::vector<int>* m_genotypeCopy;
	std::vector<int>* m_missingCustomers;
};