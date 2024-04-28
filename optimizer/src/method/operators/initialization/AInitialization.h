
#pragma once

#include "../../../problem/SProblemEncoding.h"
#include "../../individual/MO/SMOIndividual.h"
#include "../../individual/SGenotype.h"
#include "../../individual/SO/SSOIndividual.h"
#include "../../individual/SO/SParticle.h"
#include "../../../problem/problems/ECVRPTW/CECVRPTW.h"

class AInitialization
{
public:
    virtual ~AInitialization() = default;

    virtual SSOIndividual* CreateSOIndividual(SProblemEncoding &encoding) = 0;
    virtual SSOIndividual* CreateSOIndividual(SProblemEncoding &encoding, SGenotype& genotype) = 0;
    virtual SMOIndividual* CreateMOIndividual(SProblemEncoding &encoding) = 0;
    virtual SMOIndividual* CreateMOIndividualForECVRPTW(SProblemEncoding& encoding, CECVRPTW& ecvrptwProblem) = 0;
    virtual SSOIndividual* CreateNeighborSolution(SProblemEncoding &encoding, const SSOIndividual &baseSolution) = 0;
    virtual SParticle* CreateParticle(SProblemEncoding &encoding) = 0;
};
