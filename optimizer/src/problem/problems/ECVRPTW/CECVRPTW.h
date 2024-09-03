#pragma once

#include "CECVRPTWTemplate.h"
#include "../../AProblem.h"
#include "method/individual/SGenotype.h"
#include <iterator>

class CECVRPTW : public AProblem
{
public:
    explicit CECVRPTW(CECVRPTWTemplate& cvrpBase);

    SProblemEncoding& GetProblemEncoding() override { return m_ProblemEncoding; }

    void Evaluate(AIndividual& individual) override;
    void LogSolution(AIndividual& individual) override;

    void LogAdditionalData();

    CECVRPTWTemplate& GetECVRPTWTemplate() { return m_ECVRPTWTemplate; }
    std::vector<int> GetRealPath(AIndividual& individual);

protected:
    std::vector<size_t> m_UpperBounds;
    SProblemEncoding m_ProblemEncoding;
    CECVRPTWTemplate& m_ECVRPTWTemplate;
    std::vector<float> m_MaxObjectiveValues;
    std::vector<float> m_MinObjectiveValues;

private:
    void CreateProblemEncoding();
};
