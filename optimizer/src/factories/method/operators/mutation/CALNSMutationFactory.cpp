#include <regex>
#include <cstring>
#include "CALNSMutationFactory.h"
#include "../../../../method/operators/mutation/mutations/CECVRPTWRandomClientRemoval.h"
#include "../../../../method/operators/mutation/mutations/CECVRPTWShawClientRemoval.h"
#include "../../../../method/operators/mutation/mutations/CECVRPTWWorstClientRemoval.h"
#include "../../../../method/operators/mutation/mutations/CECVRPTWGreedyClientInsertion.h"
#include "../../../../method/operators/mutation/mutations/CECVRPTWShawClientInsertion.h"
#include "../../../../method/operators/mutation/mutations/CECVRPTWRandomClientInsertion.h"

std::vector<AMutation*>* CALNSMutationFactory::CreateRemovalOperators(AProblem& problem, std::vector<float>& objectiveWeights)
{
    auto randomClientRemoval = new CECVRPTWRandomClientRemoval(problem);
    auto shawClientRemoval = new CECVRPTWShawClientRemoval(problem);
    auto worstClientRemoval = new CECVRPTWWorstClientRemoval(problem, objectiveWeights);
    return new std::vector<AMutation*>{ randomClientRemoval, shawClientRemoval };
}

std::vector<AMutation*>* CALNSMutationFactory::CreateInsertionOperators(AProblem& problem, std::vector<float>& objectiveWeights)
{
    auto greedyClientInsertion = new CECVRPTWGreedyClientInsertion(problem, objectiveWeights);
    auto shawClientInsertion = new CECVRPTWShawClientInsertion(problem);
    auto randomClientInsertion = new CECVRPTWRandomClientInsertion(problem);
    return new std::vector<AMutation*>{ shawClientInsertion, randomClientInsertion };
}