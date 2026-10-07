#pragma once

#include "CCVRPTemplate.h"
#include "../../AProblem.h"
#include "../../../method/individual/SGenotype.h"
#include <cstddef>

class CCVRP : public AProblem {
public:
    explicit CCVRP(CCVRPTemplate* cvrpBase);

    ~CCVRP() {
        delete m_CVRPTemplate;
    }

    SProblemEncoding& GetProblemEncoding() override;

    void Evaluate(AIndividual& individual) override;
    void LogSolution(AIndividual& individual) override;
    void LogAdditionalData() override {};
    
    float GetOptimalValue();

    CCVRPTemplate* GetCVRPTemplate() { return m_CVRPTemplate; }
    std::size_t GetNearestDepotIdx(std::size_t cityIdx);

protected:
    std::vector<std::size_t> m_UpperBounds;
    SProblemEncoding m_ProblemEncoding;
    CCVRPTemplate* m_CVRPTemplate;
    std::vector<float> m_MaxObjectiveValues;
    std::vector<float> m_MinObjectiveValues;

private:
    void CreateProblemEncoding();
};
