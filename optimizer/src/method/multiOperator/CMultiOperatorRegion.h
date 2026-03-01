#pragma once

#include <vector>
#include "SAtomicOperatorData.h"
//#include "method/multiOperator/AMultiOperator.h"

//class AMutation;

class CMultiOperatorRegion
{
public:
    std::vector<float> m_RegionNormalizedDirection;
    //AMultiOperator<AMutation>* m_MultiMutation = nullptr;
    std::vector<SAtomicOperatorData> m_Data;

    float GetDist2ToVector(const std::vector<float>& otherVec) const;
};

