#pragma once

#include <algorithm>
#include "method/operators/mutation/AMutation.h"

class CECVRPTW;

class CECVRPTWShawClientRemoval : public AMutation
{
public:
	explicit CECVRPTWShawClientRemoval(CECVRPTW& problemDefinition);
	void Mutate(SProblemEncoding& problemEncoding, AIndividual& child) override;

private:
    CECVRPTW& m_ProblemDefinition; // TODO - should be const
	std::vector<int> m_CustomerIndexes;
};