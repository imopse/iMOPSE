#pragma once

#include "CSDVRPTemplate.h"
#include "problem/AProblem.h"

class CSDVRP : public AProblem {
    enum class EvaluationType {
        ReturnFirst,
        PathFirst
    };

public:
    explicit CSDVRP(CSDVRPTemplate &sdvrpBase);

    ~CSDVRP() override = default;

    SProblemEncoding &GetProblemEncoding() override;

    void Evaluate(AIndividual &individual) override;

    void LogSolution(AIndividual &individual) override;

    void LogAdditionalData() override {
    };

    CSDVRPTemplate &GetSDVRPTemplate() {
        return m_SDVRPTemplate;
    }

protected:
    size_t GetNearestDepotIdx(size_t cityIdx) const;

    std::vector<size_t> m_UpperBounds;
    SProblemEncoding m_ProblemEncoding;
    CSDVRPTemplate &m_SDVRPTemplate;
    std::vector<int> m_MaxObjectiveValues;
    std::vector<int> m_MinObjectiveValues;

private:
    void CreateProblemEncoding();
};
