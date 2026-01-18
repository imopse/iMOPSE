#pragma once

#include "../../../../operators/mutation/AMutation.h"
#include "../GPHHUtils.h"
#include "../individual/CGPHHIndividual.h"

class CGPHHMutation : public AMutation {
public:
    CGPHHMutation(int maxDepth, const std::vector<std::string>& terminals, 
        const std::vector<std::string>& functions, float pointMutationRate, float subtreeMutationRate);
    
    void Mutate(SProblemEncoding& problemEncoding, AIndividual &child) override;

private:
    int m_MaxDepth;
  std::vector<std::string> m_Terminals;
    std::vector<std::string> m_Functions;
    float m_PointMutationRate;
    float m_SubtreeMutationRate;
    
    void PerformPointMutation(CGPHHIndividual& ind);
    void PerformSubtreeMutation(CGPHHIndividual& ind);
};
