#pragma once

#include "method/configMap/SConfigMap.h"
#include "method/multiOperator/AMultiOperator.h"
#include "method/operators/mutation/AMutation.h"
#include "problem/AProblem.h"

class CMultiMutationFactory
{
public:
    static AMultiOperator<AMutation>* Create(SConfigMap* configMap, AProblem& problem);
};
