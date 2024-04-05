#pragma once

#include <set>
#include "../../../../method/operators/crossover/ACrossover.h"
#include "../../../../method/configMap/SConfigMap.h"
#include "../../../../problem/AProblem.h"

class CCrossoverFactory
{
public:
    static ACrossover *Create(SConfigMap *configMap, const std::string& configKey, AProblem& problem);
    static std::set<EEncodingType> GetAllEncodingTypes(const std::vector<SEncodingSection>& encoding);
};
