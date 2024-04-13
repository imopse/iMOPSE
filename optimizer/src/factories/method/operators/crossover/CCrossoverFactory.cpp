
#include <regex>
#include <cstring>
#include "CCrossoverFactory.h"
#include "method/operators/crossover/crossovers/CUniformCX.h"
#include "method/operators/crossover/crossovers/CTTP_OS_SX.h"
#include "method/operators/crossover/crossovers/CCVRP_OX.h"
#include "utils/fileReader/CReadUtils.h"

ACrossover *CCrossoverFactory::Create(SConfigMap *configMap, const std::string& configKey, AProblem& problem)
{
    std::string rawCrossoverString;
    if (!configMap->TakeValue(configKey, rawCrossoverString))
    {
        return nullptr;
    }

    auto const vec = CReadUtils::SplitLine(rawCrossoverString);
    const char *opName = vec[0].c_str();

    const std::set<EEncodingType> &encodingTypes = GetAllEncodingTypes(problem.GetProblemEncoding().m_Encoding);
    if (strcmp(opName, "UniformCX" ) == 0 && (encodingTypes.find(EEncodingType::ASSOCIATION) != encodingTypes.end() || encodingTypes.find(EEncodingType::PERMUTATION) != encodingTypes.end()))
    {
        float cxProb = std::stof(vec[1]);
        return new CUniformCX(cxProb);
    }
    else if (strcmp(opName, "TTP_OX_SX") == 0 && encodingTypes.find(EEncodingType::PERMUTATION) != encodingTypes.end())
    {
        float routeCrProb = std::stof(vec[1]);
        float knapCrProb = std::stof(vec[2]);
        return new CTTP_OS_SX(routeCrProb, knapCrProb);
    }
    else if (strcmp(opName, "CVRP_OX") == 0 && encodingTypes.find(EEncodingType::PERMUTATION) != encodingTypes.end())
    {
        float oxProb = std::stof(vec[1]);
        return new CCVRP_OX(oxProb);
    }

    return nullptr;
}

std::set<EEncodingType> CCrossoverFactory::GetAllEncodingTypes(const std::vector<SEncodingSection> &encoding)
{
    std::set<EEncodingType> uniqueTypes;

    for (const auto &section: encoding)
    {
        uniqueTypes.insert(section.m_SectionType);
    }

    return uniqueTypes;
}