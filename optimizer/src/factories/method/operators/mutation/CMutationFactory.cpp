#include <regex>
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

AMutation *CMutationFactory::Create(SConfigMap *configMap, const std::string& configKey, AProblem &problem)
{
    std::string rawMutationString;
    if (!configMap->TakeValue(configKey, rawMutationString))
    {
        return nullptr;
    }

    auto const vec = CReadUtils::SplitLine(rawMutationString);
    const char *opName = vec[0].c_str();

    const std::set<EEncodingType> &encodingTypes = GetAllEncodingTypes(problem.GetProblemEncoding().m_Encoding);
    if (strcmp(opName, "RandomBit") == 0 && (encodingTypes.find(EEncodingType::ASSOCIATION) != encodingTypes.end() || encodingTypes.find(EEncodingType::PERMUTATION) != encodingTypes.end()))
    {
        float mutProb = std::stof(vec[1]);
        return new CRandomBit(mutProb);
    }
    // CVRP
    else if (strcmp(opName, "CVRP_Reverse") == 0 && encodingTypes.find(EEncodingType::PERMUTATION) != encodingTypes.end())
    {
        float reverseMutProb = std::stof(vec[1]);
        return new CCVRPReverse(reverseMutProb);
    }
    // TTP
    else if (strcmp(opName, "TTP_Reverse") == 0 && encodingTypes.find(EEncodingType::PERMUTATION) != encodingTypes.end())
    {
        float reverseMutProb = std::stof(vec[1]);
        return new CTTPReverse(reverseMutProb);
    }
    else if (strcmp(opName, "TTP_Swap") == 0 && encodingTypes.find(EEncodingType::PERMUTATION) != encodingTypes.end())
    {
        float geneSwapProb = std::stof(vec[1]);
        return new CTTPSwap(geneSwapProb);
    }
    else if (strcmp(opName, "TTP_Flip") == 0 && encodingTypes.find(EEncodingType::PERMUTATION) != encodingTypes.end())
    {
        float flipMutProb = std::stof(vec[1]);
        return new CTTPFlip(flipMutProb);
    }
    else if (strcmp(opName, "TTP_SingleFlip") == 0 && encodingTypes.find(EEncodingType::PERMUTATION) != encodingTypes.end())
    {
        float singleFlipMutProb = std::stof(vec[1]);
        return new CTTPSingleFlip(singleFlipMutProb);
    }
    else if (strcmp(opName, "TTP_Reverse_Flip") == 0 && encodingTypes.find(EEncodingType::PERMUTATION) != encodingTypes.end())
    {
        float reverseMutProb = std::stof(vec[1]);
        float flipMutProb = std::stof(vec[2]);
        return new CTTPReverseFlip(reverseMutProb, flipMutProb);
    }
    else if (strcmp(opName, "TTP_Reverse_SingleFlip") == 0 && encodingTypes.find(EEncodingType::PERMUTATION) != encodingTypes.end())
    {
        float reverseMutProb = std::stof(vec[1]);
        float singleFlipMutProb = std::stof(vec[2]);
        return new CTTPReverseSingleFlip(reverseMutProb, singleFlipMutProb);
    }
    else if (strcmp(opName, "TTP_Swap_Flip") == 0 && encodingTypes.find(EEncodingType::PERMUTATION) != encodingTypes.end())
    {
        float geneSwapProb = std::stof(vec[1]);
        float flipMutProb = std::stof(vec[2]);
        return new CTTPSwapFlip(geneSwapProb, flipMutProb);
    }
    else if (strcmp(opName, "TTP_Swap_SingleFlip") == 0 && encodingTypes.find(EEncodingType::PERMUTATION) != encodingTypes.end())
    {
        float geneSwapProb = std::stof(vec[1]);
        float singleFlipMutProb = std::stof(vec[2]);
        return new CTTPSwapSingleFlip(geneSwapProb, singleFlipMutProb);
    }
    else if (strcmp(opName, "TTP_FullOpt2") == 0 && encodingTypes.find(EEncodingType::PERMUTATION) != encodingTypes.end())
    {
        //float  = std::stof(vec[1]); TODO - it might use probability
        return new CTTPFullOpt2(dynamic_cast<CTTP2&>(problem));
    }
    else if (strcmp(opName, "TTP_RandomSingleOpt2") == 0 && encodingTypes.find(EEncodingType::PERMUTATION) != encodingTypes.end())
    {
        //float  = std::stof(vec[1]); TODO - it might use probability
        return new CTTPRandomSingleOpt2(dynamic_cast<CTTP2&>(problem));
    }
    else if (strcmp(opName, "TTP_PickItems") == 0 && encodingTypes.find(EEncodingType::PERMUTATION) != encodingTypes.end())
    {
        float pickItemProb = std::stof(vec[1]);
        return new CTTPPickItems(pickItemProb);
    }
    else if (strcmp(opName, "TTP_DropItems") == 0 && encodingTypes.find(EEncodingType::PERMUTATION) != encodingTypes.end())
    {
        float dropItems = std::stof(vec[1]);
        return new CTTPDropItems(dropItems);
    }
    else if (strcmp(opName, "TTP_ForceLastItems") == 0 && encodingTypes.find(EEncodingType::PERMUTATION) != encodingTypes.end())
    {
        return new CTTPForceLastItems(dynamic_cast<CTTP2&>(problem));
    }
    // MSRCPSP
    else if (strcmp(opName, "CheapestResourceMutation") == 0)
    {
        float mutProb = std::stof(vec[1]);
        return new CCheapestResourceMutation(mutProb, dynamic_cast<CMSRCPSP_TA&>(problem));
    }
    else if (strcmp(opName, "LeastAssignedResourceMutation") == 0)
    {
        float mutProb = std::stof(vec[1]);
        return new CLeastAssignedResourceMutation(mutProb, dynamic_cast<CMSRCPSP_TA&>(problem));
    }
    // MSRA
    else if (strcmp(opName, "MSRA_Unassign") == 0)
    {
        float mutProb = std::stof(vec[1]);
        return new CMSRAUnassign(mutProb, dynamic_cast<CMSRAProblem&>(problem));
    }
    else if (strcmp(opName, "MSRA_ClosestTask") == 0)
    {
        float mutProb = std::stof(vec[1]);
        return new CMSRAClosestTask(mutProb, dynamic_cast<CMSRAProblem&>(problem));
    }
    else if (strcmp(opName, "MSRA_BestFit") == 0)
    {
        float mutProb = std::stof(vec[1]);
        return new CMSRABestFit(mutProb, dynamic_cast<CMSRAProblem&>(problem));
    }
    // ECVRPTW
    else if (strcmp(opName, "ECVRPTW_RandomClientRemoval") == 0)
    {
        return new CECVRPTWRandomClientRemoval(dynamic_cast<CECVRPTW&>(problem));
    }
    else if (strcmp(opName, "ECVRPTW_ShawClientRemoval") == 0)
    {
        return new CECVRPTWShawClientRemoval(dynamic_cast<CECVRPTW&>(problem));
    }
    else if (strcmp(opName, "ECVRPTW_RandomClientInsertion") == 0)
    {
        return new CECVRPTWRandomClientInsertion(dynamic_cast<CECVRPTW&>(problem));
    }
    else if (strcmp(opName, "ECVRPTW_ShawClientInsertion") == 0)
    {
        return new CECVRPTWShawClientInsertion(dynamic_cast<CECVRPTW&>(problem));
    }
    return nullptr;
}

std::set<EEncodingType> CMutationFactory::GetAllEncodingTypes(const std::vector<SEncodingSection> &encoding)
{
    std::set<EEncodingType> uniqueTypes;

    for (const auto &section: encoding)
    {
        uniqueTypes.insert(section.m_SectionType);
    }

    return uniqueTypes;
}