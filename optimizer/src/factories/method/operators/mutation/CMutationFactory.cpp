#include <cstring>
#include "CMutationFactory.h"
#include "method/operators/mutation/mutations/CRandomBit.h"
#include "method/operators/mutation/mutations/CCheapestResourceMutation.h"
#include "method/operators/mutation/mutations/CLeastAssignedResourceMutation.h"
#include "utils/fileReader/CReadUtils.h"
#include "problem/problems/MSRCPSP/CMSRCPSP_TA.h"
#include "problem/problems/TTP/CTTP2.h"
#include "problem/problems/MSRA/MSRAProblem.h"
#include "problem/problems/ECVRPTW/CECVRPTW.h"
#include "method/operators/mutation/mutations/CCVRPReverse.h"
#include "method/operators/mutation/mutations/TTP/CTTPFlip.h"
#include "method/operators/mutation/mutations/TTP/CTTPReverse.h"
#include "method/operators/mutation/mutations/TTP/CTTPSingleFlip.h"
#include "method/operators/mutation/mutations/TTP/CTTPSwap.h"
#include "method/operators/mutation/mutations/TTP/CTTPReverseFlip.h"
#include "method/operators/mutation/mutations/TTP/CTTPReverseSingleFlip.h"
#include "method/operators/mutation/mutations/TTP/CTTPSwapFlip.h"
#include "method/operators/mutation/mutations/TTP/CTTPSwapSingleFlip.h"
#include "method/operators/mutation/mutations/TTP/CTTPFullOpt2.h"
#include "method/operators/mutation/mutations/TTP/CTTPRandomSingleOpt2.h"
#include "method/operators/mutation/mutations/TTP/CTTPPickItems.h"
#include "method/operators/mutation/mutations/TTP/CTTPDropItems.h"
#include "method/operators/mutation/mutations/TTP/CTTPForceLastItems.h"
#include "method/operators/mutation/mutations/MSRA/CMSRAUnassign.h"
#include "method/operators/mutation/mutations/MSRA/CMSRAClosestTask.h"
#include "method/operators/mutation/mutations/MSRA/CMSRABestFit.h"
#include "method/operators/mutation/mutations/ECVRPTW/CECVRPTWRandomClientRemoval.h"
#include "method/operators/mutation/mutations/ECVRPTW/CECVRPTWShawClientRemoval.h"
#include "method/operators/mutation/mutations/ECVRPTW/CECVRPTWShawClientInsertion.h"
#include "method/operators/mutation/mutations/ECVRPTW/CECVRPTWRandomClientInsertion.h"
#include "method/multiOperator/operatorSelectors/CUniformMultiOperator.h"
#include "method/multiOperator/operatorSelectors/CBalancedCreditRouletteMultiOperator.h"
#include "method/multiOperator/operatorSelectors/CMaxCreditByCallsMultiOperator.h"
#include "method/multiOperator/operatorSelectors/CMaxInvCreditByCallsMultiOperator.h"
#include "method/multiOperator/operatorSelectors/CUCBMultiOperator.h"
#include "method/multiOperator/operatorSelectors/CUCB2MultiOperator.h"

static const char* const MUTATION = "Mutation";

AMutation *CMutationFactory::Create(SConfigMap *configMap, std::string key, AProblem* problem)
{
    std::string rawMutationString;
    if (!configMap->TakeValue(key, rawMutationString))
    {
        return nullptr;
    }

    auto const vec = CReadUtils::SplitLine(rawMutationString);
    if (vec.empty())
    {
        return nullptr;
    }

    const char *opName = vec[0].c_str();
    const auto &encodingTypes = problem->GetProblemEncoding().GetAllEncodingTypes();

    if (strcmp(opName, "RandomBit") == 0)
    {
        if (encodingTypes.find(EEncodingType::ASSOCIATION) != encodingTypes.end() && encodingTypes.find(EEncodingType::PERMUTATION) != encodingTypes.end())
        {
            return nullptr;
        }

        if (vec.size() != 2) { return nullptr; }

        float mutProb = std::stof(vec[1]);
        return new CRandomBit(mutProb);
    }
    else if (strcmp(opName, "CVRP_Reverse") == 0)
    {
        if (encodingTypes.find(EEncodingType::PERMUTATION) != encodingTypes.end())
        {
            return nullptr;
        }

        if (vec.size() != 2) { return nullptr; }

        float reverseMutProb = std::stof(vec[1]);
        return new CCVRPReverse(reverseMutProb);
    }
    else if (strcmp(opName, "TTP_Reverse") == 0)
    {
        if (encodingTypes.find(EEncodingType::PERMUTATION) != encodingTypes.end())
        {
            return nullptr;
        }

        if (vec.size() != 2) { return nullptr; }

        float reverseMutProb = std::stof(vec[1]);
        return new CTTPReverse(reverseMutProb);
    }
    else if (strcmp(opName, "TTP_Swap") == 0)
    {
        if (encodingTypes.find(EEncodingType::PERMUTATION) != encodingTypes.end())
        {
            return nullptr;
        }

        if (vec.size() != 2) { return nullptr; }

        float geneSwapProb = std::stof(vec[1]);
        return new CTTPSwap(geneSwapProb);
    }
    else if (strcmp(opName, "TTP_Flip") == 0)
    {
        if (encodingTypes.find(EEncodingType::PERMUTATION) != encodingTypes.end())
        {
            return nullptr;
        }

        if (vec.size() != 2) { return nullptr; }

        float flipMutProb = std::stof(vec[1]);
        return new CTTPFlip(flipMutProb);
    }
    else if (strcmp(opName, "TTP_SingleFlip") == 0)
    {
        if (encodingTypes.find(EEncodingType::PERMUTATION) != encodingTypes.end())
        {
            return nullptr;
        }

        if (vec.size() != 2) { return nullptr; }

        float singleFlipMutProb = std::stof(vec[1]);
        return new CTTPSingleFlip(singleFlipMutProb);
    }
    else if (strcmp(opName, "TTP_Reverse_Flip") == 0)
    {
        if (encodingTypes.find(EEncodingType::PERMUTATION) != encodingTypes.end())
        {
            return nullptr;
        }

        if (vec.size() != 3) { return nullptr; }

        float reverseMutProb = std::stof(vec[1]);
        float flipMutProb = std::stof(vec[2]);
        return new CTTPReverseFlip(reverseMutProb, flipMutProb);
    }
    else if (strcmp(opName, "TTP_Reverse_SingleFlip") == 0)
    {
        if (encodingTypes.find(EEncodingType::PERMUTATION) != encodingTypes.end())
        {
            return nullptr;
        }

        if (vec.size() != 3) { return nullptr; }

        float reverseMutProb = std::stof(vec[1]);
        float singleFlipMutProb = std::stof(vec[2]);
        return new CTTPReverseSingleFlip(reverseMutProb, singleFlipMutProb);
    }
    else if (strcmp(opName, "TTP_Swap_Flip") == 0)
    {
        if (encodingTypes.find(EEncodingType::PERMUTATION) != encodingTypes.end())
        {
            return nullptr;
        }

        if (vec.size() != 3) { return nullptr; }

        float geneSwapProb = std::stof(vec[1]);
        float flipMutProb = std::stof(vec[2]);
        return new CTTPSwapFlip(geneSwapProb, flipMutProb);
    }
    else if (strcmp(opName, "TTP_Swap_SingleFlip") == 0)
    {
        if (encodingTypes.find(EEncodingType::PERMUTATION) != encodingTypes.end())
        {
            return nullptr;
        }

        if (vec.size() != 3) { return nullptr; }

        float geneSwapProb = std::stof(vec[1]);
        float singleFlipMutProb = std::stof(vec[2]);
        return new CTTPSwapSingleFlip(geneSwapProb, singleFlipMutProb);
    }
    else if (strcmp(opName, "TTP_FullOpt2") == 0)
    {
        if (encodingTypes.find(EEncodingType::PERMUTATION) != encodingTypes.end())
        {
            return nullptr;
        }

        if (vec.size() != 1) { return nullptr; }

        return new CTTPFullOpt2(dynamic_cast<CTTP2&>(*problem));
    }
    else if (strcmp(opName, "TTP_RandomSingleOpt2") == 0)
    {
        if (encodingTypes.find(EEncodingType::PERMUTATION) != encodingTypes.end())
        {
            return nullptr;
        }

        if (vec.size() != 1) { return nullptr; }

        return new CTTPRandomSingleOpt2(dynamic_cast<CTTP2&>(*problem));
    }
    else if (strcmp(opName, "TTP_PickItems") == 0)
    {
        if (encodingTypes.find(EEncodingType::PERMUTATION) != encodingTypes.end())
        {
            return nullptr;
        }

        if (vec.size() != 2) { return nullptr; }

        float pickItemProb = std::stof(vec[1]);
        return new CTTPPickItems(pickItemProb);
    }
    else if (strcmp(opName, "TTP_DropItems") == 0)
    {
        if (encodingTypes.find(EEncodingType::PERMUTATION) != encodingTypes.end())
        {
            return nullptr;
        }

        if (vec.size() != 2) { return nullptr; }

        float dropItems = std::stof(vec[1]);
        return new CTTPDropItems(dropItems);
    }
    else if (strcmp(opName, "TTP_ForceLastItems") == 0)
    {
        if (encodingTypes.find(EEncodingType::PERMUTATION) != encodingTypes.end())
        {
            return nullptr;
        }

        if (vec.size() != 1) { return nullptr; }

        return new CTTPForceLastItems(dynamic_cast<CTTP2&>(*problem));
    }
    else if (strcmp(opName, "CheapestResourceMutation") == 0)
    {
        if (vec.size() != 2) { return nullptr; }

        float mutProb = std::stof(vec[1]);
        return new CCheapestResourceMutation(mutProb, dynamic_cast<CMSRCPSP_TA&>(*problem));
    }
    else if (strcmp(opName, "LeastAssignedResourceMutation") == 0)
    {
        if (vec.size() != 2) { return nullptr; }

        float mutProb = std::stof(vec[1]);
        return new CLeastAssignedResourceMutation(mutProb, dynamic_cast<CMSRCPSP_TA&>(*problem));
    }
    else if (strcmp(opName, "MSRA_Unassign") == 0)
    {
        if (vec.size() != 2) { return nullptr; }

        float mutProb = std::stof(vec[1]);
        return new CMSRAUnassign(mutProb, dynamic_cast<CMSRAProblem&>(*problem));
    }
    else if (strcmp(opName, "MSRA_ClosestTask") == 0)
    {
        if (vec.size() != 2) { return nullptr; }

        float mutProb = std::stof(vec[1]);
        return new CMSRAClosestTask(mutProb, dynamic_cast<CMSRAProblem&>(*problem));
    }
    else if (strcmp(opName, "MSRA_BestFit") == 0)
    {
        if (vec.size() != 2) { return nullptr; }

        float mutProb = std::stof(vec[1]);
        return new CMSRABestFit(mutProb, dynamic_cast<CMSRAProblem&>(*problem));
    }
    else if (strcmp(opName, "ECVRPTW_RandomClientRemoval") == 0)
    {
        if (vec.size() != 1) { return nullptr; }

        return new CECVRPTWRandomClientRemoval(dynamic_cast<CECVRPTW&>(*problem));
    }
    else if (strcmp(opName, "ECVRPTW_ShawClientRemoval") == 0)
    {
        if (vec.size() != 1) { return nullptr; }

        return new CECVRPTWShawClientRemoval(dynamic_cast<CECVRPTW&>(*problem));
    }
    else if (strcmp(opName, "ECVRPTW_RandomClientInsertion") == 0)
    {
        if (vec.size() != 1) { return nullptr; }

        return new CECVRPTWRandomClientInsertion(dynamic_cast<CECVRPTW&>(*problem));
    }
    else if (strcmp(opName, "ECVRPTW_ShawClientInsertion") == 0)
    {
        if (vec.size() != 1) { return nullptr; }

        return new CECVRPTWShawClientInsertion(dynamic_cast<CECVRPTW&>(*problem));
    }

    return nullptr;
}

AMutation *CMutationFactory::Create(SConfigMap *configMap, AProblem* problem) {
    return CMutationFactory::Create(configMap, MUTATION, problem);
}

AMultiOperator<AMutation>* CMutationFactory::CreateMultiMutation(SConfigMap* configMap, AProblem* problem)
{
    std::string rawString;
    if (!configMap->TakeValue("MultiMutation", rawString))
    {
        return nullptr;
    }

    auto const vec = CReadUtils::SplitLine(rawString);
    if (vec.empty())
    {
        return nullptr;
    }

    const char *opName = vec[0].c_str();

    AMultiOperator<AMutation>* multiOperator = nullptr;
    if (strcmp(opName, "UniformMultiOperator") == 0)
    {
        multiOperator = new CUniformMultiOperator<AMutation>();
    }
    else if (strcmp(opName, "BalancedCreditRouletteMultiOperator") == 0)
    {
        multiOperator = new CBalancedCreditRouletteMultiOperator<AMutation>();
    }
    else if (strcmp(opName, "CreditRouletteMultiOperator") == 0)
    {
        multiOperator = new CBalancedCreditRouletteMultiOperator<AMutation>();
    }
    else if (strcmp(opName, "InvCreditRouletteMultiOperator") == 0)
    {
        multiOperator = new CBalancedCreditRouletteMultiOperator<AMutation>();
    }
    else if (strcmp(opName, "MaxCreditByCallsMultiOperator") == 0)
    {
        multiOperator = new CMaxCreditByCallsMultiOperator<AMutation>();
    }
    else if (strcmp(opName, "MaxInvCreditByCallsMultiOperator") == 0)
    {
        multiOperator = new CMaxInvCreditByCallsMultiOperator<AMutation>();
    }
    else if (strcmp(opName, "UCBMultiOperator") == 0)
    {
        multiOperator = new CUCBMultiOperator<AMutation>();
    }
    else if (strcmp(opName, "UCB2MultiOperator") == 0)
    {
        multiOperator = new CUCB2MultiOperator<AMutation>();
    }

    if (multiOperator)
    {
        CMutationFactory mutationFactory;
        const std::string subMutationKey("SubMutation");
        int subOperatorIdx = 0;
        std::string nextSubMutationKey = subMutationKey + std::to_string(subOperatorIdx);
        while (configMap->HasValue(nextSubMutationKey))
        {
            if (AMutation* atomicOperator = mutationFactory.Create(configMap, nextSubMutationKey, problem))
            {
                multiOperator->AddOperator(atomicOperator);
            }
            nextSubMutationKey = subMutationKey + std::to_string(++subOperatorIdx);
        }
    }

    return multiOperator;
}

std::vector<AMutation*>* CMutationFactory::CreateRemovalOperators(AProblem* problem)
{
    auto randomClientRemoval = new CECVRPTWRandomClientRemoval(dynamic_cast<CECVRPTW&>(*problem));
    auto shawClientRemoval = new CECVRPTWShawClientRemoval(dynamic_cast<CECVRPTW&>(*problem));
    return new std::vector<AMutation*>{ randomClientRemoval, shawClientRemoval };
}

std::vector<AMutation*>* CMutationFactory::CreateInsertionOperators(AProblem* problem)
{
    auto greedyClientInsertion = new CECVRPTWRandomClientInsertion(dynamic_cast<CECVRPTW&>(*problem));
    auto shawClientInsertion = new CECVRPTWShawClientInsertion(dynamic_cast<CECVRPTW&>(*problem));
    return new std::vector<AMutation*>{ greedyClientInsertion, shawClientInsertion };
}