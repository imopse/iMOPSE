#pragma once

#include "../../../../method/operators/mutation/AMutation.h"
#include "../../../../method/configMap/SConfigMap.h"
#include "../../../../problem/AProblem.h"
#include "method/multiOperator/AMultiOperator.h"
#include <set>

class CMutationFactory
{
public:
    static AMutation* Create(SConfigMap* configMap, std::string key, AProblem* problem);
    static AMutation* Create(SConfigMap* configMap, AProblem* problem);
    static AMultiOperator<AMutation>* CreateMultiMutation(SConfigMap* configMap, AProblem* problem);
    static std::vector<AMutation* >* CreateRemovalOperators(AProblem* problem);
    static std::vector<AMutation* >* CreateInsertionOperators(AProblem* problem);
};