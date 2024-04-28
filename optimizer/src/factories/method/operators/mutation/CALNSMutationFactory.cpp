#include <regex>
#include <cstring>
#include "CALNSMutationFactory.h"
#include "../../../../method/operators/mutation/mutations/CECVRPTWRandomClientRemoval.h"
#include "../../../../method/operators/mutation/mutations/CECVRPTWShawClientRemoval.h"
#include "../../../../method/operators/mutation/mutations/CECVRPTWWorstClientRemoval.h"
#include "../../../../method/operators/mutation/mutations/CECVRPTWGreedyClientInsertion.h"
#include "../../../../method/operators/mutation/mutations/CECVRPTWShawClientInsertion.h"

std::vector<AMutation*>* CALNSMutationFactory::CreateRemovalOperators(AProblem& problem)
{
    auto randomClientRemoval = new CECVRPTWRandomClientRemoval(problem);
    auto shawClientRemoval = new CECVRPTWShawClientRemoval(problem);
    auto worstClientRemoval = new CECVRPTWWorstClientRemoval(problem);
    return new std::vector<AMutation*>{ randomClientRemoval, shawClientRemoval, worstClientRemoval };
}

std::vector<AMutation*>* CALNSMutationFactory::CreateInsertionOperators(AProblem& problem)
{
    auto greedyClientInsertion = new CECVRPTWGreedyClientInsertion(problem);
    auto shawClientInsertion = new CECVRPTWShawClientInsertion(problem);
    return new std::vector<AMutation*>{ greedyClientInsertion, shawClientInsertion };
}