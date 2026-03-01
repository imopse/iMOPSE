#include "CTTPForceLastItems.h"
#include "problem/problems/TTP/CTTP2.h"

void CTTPForceLastItems::Mutate(SProblemEncoding& problemEncoding, AIndividual& child)
{
    // Route Greedy Search
    m_ProblemDefinition.PickMostValItemsFromTheEnd(child);
}
