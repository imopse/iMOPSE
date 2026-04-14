#include "CCrossoverFactory.h"
#include "method/operators/crossover/crossovers/CUniformCX.h"
#include "method/operators/crossover/crossovers/CTTP_OS_SX.h"
#include "method/operators/crossover/crossovers/CMSRA_RX.h"
#include "problem/problems/MSRA/MSRAProblem.h"
#include "utils/fileReader/CReadUtils.h"
#include "method/operators/crossover/crossovers/CCVRP_OX.h"

static const char* const CROSSOVER = "Crossover";

ACrossover *CCrossoverFactory::Create(SConfigMap *configMap, AProblem* problem)
{
    std::string crossoverData;
    if (!configMap->TakeValue(CROSSOVER, crossoverData))
    {
        return nullptr;
    }

    auto const vec = CReadUtils::SplitLine(crossoverData);
    const char *opName = vec[0].c_str();

    const auto &encodingTypes = problem->GetProblemEncoding().GetAllEncodingTypes();
    if (vec[0] == "UniformCX" )
    {
        float cxProb = std::stof(vec[1]);
        return new CUniformCX(cxProb);
    }
    else if (vec[0] == "TTP_OX_SX" && encodingTypes.find(EEncodingType::PERMUTATION) != encodingTypes.end())
    {
        float routeCrProb = std::stof(vec[1]);
        float knapCrProb = std::stof(vec[2]);
        return new CTTP_OS_SX(routeCrProb, knapCrProb);
    }
    else if (vec[0] == "CVRP_OX" && encodingTypes.find(EEncodingType::PERMUTATION) != encodingTypes.end())
    {
        float oxProb = std::stof(vec[1]);
        return new CCVRP_OX(oxProb);
    }
    else if (vec[0] == "MSRA_RX" && encodingTypes.find(EEncodingType::PERMUTATION) != encodingTypes.end())
    {
        float rxProb = std::stof(vec[1]);
        return new CMSRA_RX(rxProb, dynamic_cast<CMSRAProblem&>(*problem));
    }

    return nullptr;
}