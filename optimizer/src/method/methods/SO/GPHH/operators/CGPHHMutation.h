#pragma once

#include "../../../../operators/mutation/AMutation.h"
#include "../GPHHUtils.h"
#include "../individual/CGPHHIndividual.h"

class CGPHHMutation : public AMutation {
public:
    CGPHHMutation(int maxDepth, const std::vector<std::string>& terminals, 
        const std::vector<std::string>& functions, float pointMutationRate, float subtreeMutationRate);
    
    void Mutate(SProblemEncoding& problemEncoding, AIndividual &child) override;

    size_t GetParamCount() const override { return 1; }
    float* GetParamValue(int paramIdx) override { return paramIdx == 0 ? &m_PointMutationRate : nullptr; }

private:
    int m_MaxDepth;
  std::vector<std::string> m_Terminals;
    std::vector<std::string> m_Functions;
    float m_PointMutationRate;
    float m_SubtreeMutationRate;
    
    void PerformPointMutation(CGPHHIndividual& ind);
    void PerformSubtreeMutation(CGPHHIndividual& ind);
};
