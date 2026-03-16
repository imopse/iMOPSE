#pragma once

#include "../../../../individual/SO/SSOIndividual.h"
#include "../nodes/AGPHHNode.h"
#include "../nodes/functions/CFunctions.h"
#include <vector>

class CGPHHIndividual : public SSOIndividual {
public:
    CGPHHIndividual();
    CGPHHIndividual(const CGPHHIndividual& other);
    ~CGPHHIndividual() override;
    AGPHHNode* m_Root;

    void Clear();
    
    // Flatten tree to postorder for iterative evaluation
    void FlattenTree();
    
    // Iterative evaluation (faster than recursive)
    float EvaluateIterative(const SGPHHContext& context);

    
private:
    std::vector<AGPHHNode*> m_PostOrderNodes;
    bool m_IsFlattened;
    
    void PostOrderTraversal(AGPHHNode* node, std::vector<AGPHHNode*>& nodes);
    

};
