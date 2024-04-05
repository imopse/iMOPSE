
#pragma once

#include <vector>
#include "../../../problem/SProblemEncoding.h"
#include "../../individual/AIndividual.h"

class ACrossover
{
public:
    virtual ~ACrossover() = default;

    virtual void Crossover(
            const SProblemEncoding& problemEncoding,
            AIndividual &firstParent,
            AIndividual &secondParent,
            AIndividual &firstChild,
            AIndividual &secondChild) = 0;
};
