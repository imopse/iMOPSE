#pragma once

#include "../../SProblemEncoding.h"
#include "../../../method/individual/SGenotype.h"
#include "../../AProblem.h"
#include "MSRAInstance.h"


class CMSRAProblem : public AProblem
{
public:
    explicit CMSRAProblem(CMSRAInstance& problemTemplate);

    SProblemEncoding& GetProblemEncoding() override;
    void Evaluate(AIndividual& individual) override;
    void LogSolution(AIndividual& individual) override;

    float GetUnassignGeneValue() const;
    float FindBestGeneValueByClosestTask(const std::vector<float>& solution, const size_t geneIdx) const;
    float FindBestGeneValueByProb(const size_t geneIdx) const;

    size_t GetStageCount() const { return m_ProblemTemplate.GetStageCount(); }

private:

    void EvaluateWithoutFix(AIndividual& individual);
    void EvaluateWithFix(AIndividual& individual);

	void PrepareEncoding();
	bool CanResourceWorkOnTask(size_t resourceIdx, size_t taskIdx, size_t stage, size_t taskCount, size_t lastResourceTask, int lastResourceWorkStage) const;

    float CalcMinEvalValue(size_t objIdx) const;
    float CalcMaxEvalValue(size_t objIdx) const;

	SProblemEncoding m_ProblemEncoding;
    std::vector<float> m_MinObjectiveValues;
    std::vector<float> m_MaxObjectiveValues;

	CMSRAInstance m_ProblemTemplate;
};