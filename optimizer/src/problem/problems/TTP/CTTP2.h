#pragma once

#include "CTTPTemplate.h"
#include "../../AProblem.h"
#include "../../../method/individual/SGenotype.h"

class CTTP2 : public AProblem
{
public:
    explicit CTTP2(CTTPTemplate &ttpBase);
    ~CTTP2() override = default;

    SProblemEncoding &GetProblemEncoding() override;
    void Evaluate(AIndividual& individual) override;
    void LogSolution(AIndividual& individual) override;

protected:

    std::vector<std::vector<size_t>> m_CityItems;
    std::vector<size_t> m_UpperBounds;
    SProblemEncoding m_ProblemEncoding;
    CTTPTemplate &m_TTPTemplate;
    std::vector<float> m_MaxObjectiveValues;
    std::vector<float> m_MinObjectiveValues;

private:
    void CreateProblemEncoding();
};