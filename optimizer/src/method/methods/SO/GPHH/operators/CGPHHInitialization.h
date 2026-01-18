#pragma once

#include "../../../../operators/initialization/AInitialization.h"
#include "../GPHHUtils.h"
#include "../individual/CGPHHIndividual.h"

class CGPHHInitialization : public AInitialization {
public:
    CGPHHInitialization(int maxDepth, const std::vector<std::string>& terminals, const std::vector<std::string>& functions);
    
    SSOIndividual* CreateSOIndividual(SProblemEncoding &encoding) override;
    SSOIndividual* CreateSOIndividual(SProblemEncoding &encoding, SGenotype& genotype) override;
    SMOIndividual* CreateMOIndividual(SProblemEncoding &encoding) override;
    SSOIndividual* CreateNeighborSolution(SProblemEncoding &encoding, const SSOIndividual &baseSolution) override;
    SParticle* CreateParticle(SProblemEncoding &encoding) override;

private:
    int m_MaxDepth;
    std::vector<std::string> m_Terminals;
    std::vector<std::string> m_Functions;
};
