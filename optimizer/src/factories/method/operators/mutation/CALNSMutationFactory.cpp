#include <regex>
#include <cstring>
#include "CALNSMutationFactory.h"
#include "method/operators/mutation/mutations/ECVRPTW/CECVRPTWRandomClientRemoval.h"
#include "method/operators/mutation/mutations/ECVRPTW/CECVRPTWShawClientRemoval.h"
#include "method/operators/mutation/mutations/ECVRPTW/CECVRPTWRandomClientInsertion.h"
#include "method/operators/mutation/mutations/ECVRPTW/CECVRPTWShawClientInsertion.h"
#include "problem/problems/ECVRPTW/CECVRPTW.h"

std::vector<AMutation*>* CALNSMutationFactory::CreateRemovalOperators(AProblem& problem)
{
    auto randomClientRemoval = new CECVRPTWRandomClientRemoval(dynamic_cast<CECVRPTW&>(problem));
    auto shawClientRemoval = new CECVRPTWShawClientRemoval(dynamic_cast<CECVRPTW&>(problem));
    return new std::vector<AMutation*>{ randomClientRemoval, shawClientRemoval };
}

std::vector<AMutation*>* CALNSMutationFactory::CreateInsertionOperators(AProblem& problem)
{
    auto greedyClientInsertion = new CECVRPTWRandomClientInsertion(dynamic_cast<CECVRPTW&>(problem));
    auto shawClientInsertion = new CECVRPTWShawClientInsertion(dynamic_cast<CECVRPTW&>(problem));
    return new std::vector<AMutation*>{ greedyClientInsertion, shawClientInsertion };
}