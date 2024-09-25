#pragma once

#include <algorithm>
#include <chrono>
#include "method/operators/mutation/AMutation.h"

class CECVRPTW;

class CECVRPTWShawClientInsertion : public AMutation
{
public:
	explicit CECVRPTWShawClientInsertion(CECVRPTW& problemDefinition);
	void Mutate(SProblemEncoding& problemEncoding, AIndividual& child) override;

private:
    CECVRPTW& m_ProblemDefinition; // TODO - should be const
};