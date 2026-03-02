#include "CGPHHCrossover.h"
#include "utils/random/CRandom.h"

CGPHHCrossover::CGPHHCrossover(int maxDepth, float crossoverRate) 
    : m_MaxDepth(maxDepth), m_CrossoverRate(crossoverRate) {}

void CGPHHCrossover::Crossover(
        const SProblemEncoding& problemEncoding,
        AIndividual &firstParent,
        AIndividual &secondParent,
        AIndividual &firstChild,
        AIndividual &secondChild) {
    
    if (CRandom::GetFloat(0, 1) > m_CrossoverRate) {
        return;
    }
    
    CGPHHIndividual& p1 = static_cast<CGPHHIndividual&>(firstParent);
    CGPHHIndividual& p2 = static_cast<CGPHHIndividual&>(secondParent);
    CGPHHIndividual& c1 = static_cast<CGPHHIndividual&>(firstChild);
    CGPHHIndividual& c2 = static_cast<CGPHHIndividual&>(secondChild);
    
    std::vector<GPHHUtils::NodeInfo> nodes1;
    std::vector<GPHHUtils::NodeInfo> nodes2;
    
    GPHHUtils::CollectNodesInfo(c1.m_Root, nullptr, false, nodes1);
    GPHHUtils::CollectNodesInfo(c2.m_Root, nullptr, false, nodes2);
    
    if (nodes1.empty() || nodes2.empty()) return;
    
    int idx1 = CRandom::GetInt(0, nodes1.size());
    int idx2 = CRandom::GetInt(0, nodes2.size());
    
    AGPHHNode* subtree1 = nodes1[idx1].node;
    AGPHHNode* subtree2 = nodes2[idx2].node;
    
    // Update parent 1 to point to subtree 2
    if (nodes1[idx1].parent == nullptr) {
        c1.m_Root = subtree2;
    } else {
        CFunction* parent = dynamic_cast<CFunction*>(nodes1[idx1].parent);
        if (nodes1[idx1].isLeftChild) parent->SetLeft(subtree2);
        else parent->SetRight(subtree2);
    }
    
    // Update parent 2 to point to subtree 1
    if (nodes2[idx2].parent == nullptr) {
        c2.m_Root = subtree1;
    } else {
        CFunction* parent = dynamic_cast<CFunction*>(nodes2[idx2].parent);
        if (nodes2[idx2].isLeftChild) parent->SetLeft(subtree1);
        else parent->SetRight(subtree1);
    }
}
