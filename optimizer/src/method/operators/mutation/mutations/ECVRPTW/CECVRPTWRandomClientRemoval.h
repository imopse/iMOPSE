#pragma once

#include <algorithm>
#include "method/operators/mutation/AMutation.h"

class CECVRPTW;

class CECVRPTWRandomClientRemoval : public AMutation
{
public:
	explicit CECVRPTWRandomClientRemoval(CECVRPTW& problemDefinition);
	void Mutate(SProblemEncoding& problemEncoding, AIndividual& child) override;

private:
    CECVRPTW& m_ProblemDefinition; // TODO - should be const
};