#pragma once

#include "../AInitialization.h"
#include "../../../individual/SO/SSOIndividual.h"

class CInitialization : public AInitialization
{
public:
    ~CInitialization() override = default;
    
    SSOIndividual* CreateSOIndividual(SProblemEncoding &encoding) override;
    SSOIndividual* CreateSOIndividual(SProblemEncoding &encoding, SGenotype& genotype) override;
    SMOIndividual* CreateMOIndividual(SProblemEncoding &encoding) override;
    SMOIndividual* CreateMOIndividualForECVRPTW(SProblemEncoding &encoding, CECVRPTW& ecvrptwProblem) override;
    SSOIndividual* CreateNeighborSolution(SProblemEncoding &encoding, const SSOIndividual &baseSolution) override;
    SParticle* CreateParticle(SProblemEncoding &encoding) override;
private:
    void InitGenotype(SProblemEncoding &encoding, SGenotype &genotype) const;
};
