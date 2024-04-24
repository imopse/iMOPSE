#pragma once

#include "CCVRPTemplate.h"
#include "../../AProblem.h"
#include "../../../method/individual/SGenotype.h"

class CCVRP : public AProblem {
public:
    explicit CCVRP(CCVRPTemplate& cvrpBase);

    ~CCVRP() override = default;

    SProblemEncoding& GetProblemEncoding() override;

    void Evaluate(AIndividual& individual) override;
    void LogSolution(AIndividual& individual) override;
    void LogAdditionalData() override {};

protected:
    size_t GetNearestDepotIdx(size_t cityIdx);

    std::vector<size_t> m_UpperBounds;
    SProblemEncoding m_ProblemEncoding;
    CCVRPTemplate& m_CVRPTemplate;
    std::vector<float> m_MaxObjectiveValues;
    std::vector<float> m_MinObjectiveValues;

private:
    void CreateProblemEncoding();
};
