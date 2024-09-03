#pragma once

#include <algorithm>
#include <chrono>
#include "method/operators/mutation/AMutation.h"

class CECVRPTW;

class CECVRPTWRandomClientInsertion : public AMutation
{
public:
	explicit CECVRPTWRandomClientInsertion(CECVRPTW& problemDefinition);
	void Mutate(SProblemEncoding& problemEncoding, AIndividual& child) override;

private:
    CECVRPTW& m_ProblemDefinition; // TODO - should be const
};