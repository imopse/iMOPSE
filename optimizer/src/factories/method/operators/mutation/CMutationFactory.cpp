#include <regex>
#include <cstring>
#include "CMutationFactory.h"
#include "method/operators/mutation/mutations/CRandomBit.h"
#include "method/operators/mutation/mutations/CTTPReverseFlip.h"
#include "method/operators/mutation/mutations/CCVRPReverseFlip.h"
#include "method/operators/mutation/mutations/CCheapestResourceMutation.h"
#include "method/operators/mutation/mutations/CLeastAssignedResourceMutation.h"
#include "utils/fileReader/CReadUtils.h"
#include "problem/problems/MSRCPSP/CMSRCPSP_TA.h"

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
    else if (strcmp(opName, "TTP_Reverse_Flip") == 0 && encodingTypes.find(EEncodingType::PERMUTATION) != encodingTypes.end())
    {
        float mutProb = std::stof(vec[1]);
        float etaCoef = std::stof(vec[2]);
        return new CTTPReverseFlip(mutProb, etaCoef);
    }
    else if (strcmp(opName, "CVRP_Reverse_Flip") == 0 && encodingTypes.find(EEncodingType::PERMUTATION) != encodingTypes.end())
    {
        float mutProb = std::stof(vec[1]);
        return new CCVRPReverseFlip(mutProb);
    }
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