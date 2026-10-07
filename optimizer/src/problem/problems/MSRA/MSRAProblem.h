#pragma once

#include "../../SProblemEncoding.h"
#include "../../../method/individual/SGenotype.h"
#include "../../AProblem.h"
#include "MSRAInstance.h"
#include <cstddef>


class CMSRAProblem : public AProblem
{
public:
    explicit CMSRAProblem(CMSRAInstance* problemTemplate);
	~CMSRAProblem() {
		delete m_ProblemTemplate;
	}

    SProblemEncoding& GetProblemEncoding() override;
    void Evaluate(AIndividual& individual) override;
    void LogSolution(AIndividual& individual) override;
    void LogAdditionalData() override {};

    float GetUnassignGeneValue() const;
    float FindBestGeneValueByClosestTask(const std::vector<float>& solution, const std::size_t geneIdx) const;
    float FindBestGeneValueByProb(const std::size_t geneIdx) const;

    std::size_t GetStageCount() const { return m_ProblemTemplate->GetStageCount(); }

private:

    void EvaluateWithoutFix(AIndividual& individual);
    void EvaluateWithFix(AIndividual& individual);

	void PrepareEncoding();
	bool CanResourceWorkOnTask(std::size_t resourceIdx, std::size_t taskIdx, std::size_t stage, std::size_t taskCount, std::size_t lastResourceTask, int lastResourceWorkStage) const;

    float CalcMinEvalValue(std::size_t objIdx) const;
    float CalcMaxEvalValue(std::size_t objIdx) const;

	SProblemEncoding m_ProblemEncoding;
    std::vector<float> m_MinObjectiveValues;
    std::vector<float> m_MaxObjectiveValues;

	CMSRAInstance* m_ProblemTemplate;
};
