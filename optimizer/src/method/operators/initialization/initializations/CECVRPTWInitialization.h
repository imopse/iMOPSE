#pragma once

#include "../AInitialization.h"
#include "../../../individual/SO/SSOIndividual.h"
#include "../../../../problem/problems/ECVRPTW/CECVRPTW.h"

class CECVRPTWInitialization : public AInitialization
{
public:
    CECVRPTWInitialization(CECVRPTW& problem) : m_problem(problem) {}
    ~CECVRPTWInitialization() override = default;

    SSOIndividual* CreateSOIndividual(SProblemEncoding& encoding) override;
    SSOIndividual* CreateSOIndividual(SProblemEncoding& encoding, SGenotype& genotype) override;
    SMOIndividual* CreateMOIndividual(SProblemEncoding& encoding) override;
    SSOIndividual* CreateNeighborSolution(SProblemEncoding& encoding, const SSOIndividual& baseSolution) override;
    SParticle* CreateParticle(SProblemEncoding& encoding) override;
private:
    void InitGenotype(SProblemEncoding& encoding, SGenotype& genotype) const;
    CECVRPTW& m_problem;
};
