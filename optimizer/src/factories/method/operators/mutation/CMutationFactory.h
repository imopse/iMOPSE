#pragma once

#include "../../../../method/operators/mutation/AMutation.h"
#include "../../../../method/configMap/SConfigMap.h"
#include "../../../../problem/AProblem.h"
#include <set>

class CMutationFactory
{
public:
    static AMutation *Create(SConfigMap *configMap, const std::string& configKey, AProblem& problem);
    static std::set<EEncodingType> GetAllEncodingTypes(const std::vector<SEncodingSection>& encoding);
};
