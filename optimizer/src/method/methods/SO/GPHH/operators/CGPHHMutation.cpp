#include "CGPHHMutation.h"
#include "utils/random/CRandom.h"

CGPHHMutation::CGPHHMutation(int maxDepth, const std::vector<std::string>& terminals, 
    const std::vector<std::string>& functions, float pointMutationRate, float subtreeMutationRate)
    : m_MaxDepth(maxDepth), m_Terminals(terminals), m_Functions(functions),
      m_PointMutationRate(pointMutationRate), m_SubtreeMutationRate(subtreeMutationRate) {
}

void CGPHHMutation::Mutate(SProblemEncoding& problemEncoding, AIndividual &child) {
    CGPHHIndividual& ind = static_cast<CGPHHIndividual&>(child);
    
    float randomValue = CRandom::GetFloat(0, 1);
    
    if (randomValue < m_PointMutationRate) {
        PerformPointMutation(ind);
    } else if (randomValue < m_PointMutationRate + m_SubtreeMutationRate) {
        PerformSubtreeMutation(ind);
    }
    
    GPHHUtils::PruneTreeToDepth(ind.m_Root, 0, m_MaxDepth, m_Terminals);
}

void CGPHHMutation::PerformPointMutation(CGPHHIndividual& ind) {
    std::vector<GPHHUtils::NodeInfo> nodes;
    GPHHUtils::CollectNodesInfo(ind.m_Root, nullptr, false, nodes);
    
    if (nodes.empty()) return;
    
    int idx = CRandom::GetInt(0, nodes.size());
    GPHHUtils::PointMutateNode(ind.m_Root, nodes[idx], m_Terminals, m_Functions);
}

void CGPHHMutation::PerformSubtreeMutation(CGPHHIndividual& ind) {
    std::vector<GPHHUtils::NodeInfo> nodes;
    GPHHUtils::CollectNodesInfo(ind.m_Root, nullptr, false, nodes);
    
    if (nodes.empty()) return;
    
    int idx = CRandom::GetInt(0, nodes.size());
    
    int currentDepth = nodes[idx].depth;
    int remainingDepth = m_MaxDepth - currentDepth;
    
    int mutationDepth = 2 + CRandom::GetInt(0, 3);
    if (mutationDepth > remainingDepth) mutationDepth = remainingDepth;
    if (mutationDepth < 1) mutationDepth = 1;
    
    AGPHHNode* newSubtree = GPHHUtils::GenerateRandomTree(mutationDepth, m_Terminals, m_Functions, false);
  
    GPHHUtils::ReplaceNode(ind.m_Root, nodes[idx], newSubtree);
}
