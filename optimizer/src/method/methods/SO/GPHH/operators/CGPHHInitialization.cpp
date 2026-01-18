#include "CGPHHInitialization.h"

CGPHHInitialization::CGPHHInitialization(int maxDepth, const std::vector<std::string>& terminals, const std::vector<std::string>& functions)
    : m_MaxDepth(maxDepth), m_Terminals(terminals), m_Functions(functions) {
}

SSOIndividual* CGPHHInitialization::CreateSOIndividual(SProblemEncoding &encoding) {
    CGPHHIndividual* ind = new CGPHHIndividual();
    ind->m_Genotype = SGenotype(); 
    ind->m_Evaluation.resize(1);
    ind->m_NormalizedEvaluation.resize(1);
    
    ind->m_Root = GPHHUtils::GenerateRandomTree(m_MaxDepth, m_Terminals, m_Functions, false);
    
    return ind;
}

SSOIndividual* CGPHHInitialization::CreateSOIndividual(SProblemEncoding &encoding, SGenotype& genotype) {

    return CreateSOIndividual(encoding);
}

SMOIndividual* CGPHHInitialization::CreateMOIndividual(SProblemEncoding &encoding) {
    return nullptr; // Not supported
}

SSOIndividual* CGPHHInitialization::CreateNeighborSolution(SProblemEncoding &encoding, const SSOIndividual &baseSolution) {
    return nullptr; // Not supported
}

SParticle* CGPHHInitialization::CreateParticle(SProblemEncoding &encoding) {
    return nullptr; // Not supported
}
