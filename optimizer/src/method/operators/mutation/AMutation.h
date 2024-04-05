
#pragma once

#include "../../../problem/SProblemEncoding.h"
#include "../../individual/AIndividual.h"

class AMutation
{
public:
    virtual ~AMutation() = default;

    virtual void Mutate(SProblemEncoding& problemEncoding, AIndividual &child) = 0;
};
