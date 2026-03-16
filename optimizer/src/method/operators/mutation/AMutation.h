
#pragma once

#include "../../../problem/SProblemEncoding.h"
#include "../../individual/AIndividual.h"

class AMutation
{
public:
    virtual ~AMutation() = default;

    virtual void Mutate(SProblemEncoding& problemEncoding, AIndividual &child) = 0;

    // TODO - for now we cover only mutation
    virtual size_t GetParamCount() const = 0;
    virtual float* GetParamValue(int paramIdx) = 0;
};
