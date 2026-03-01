#include "CMultiOperatorRegion.h"


float CMultiOperatorRegion::GetDist2ToVector(const std::vector<float>& otherVec) const
{
    float dist2 = 0.f;
    for (size_t i = 0; i < m_RegionNormalizedDirection.size(); ++i)
    {
        dist2 += powf(m_RegionNormalizedDirection[i] - otherVec[i], 2);
    }
    return dist2;
}