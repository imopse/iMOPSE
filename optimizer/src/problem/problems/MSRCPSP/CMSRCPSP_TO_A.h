#pragma once

#include "../../AProblem.h"
#include "CResource.h"
#include "CScheduler.h"

class CMSRCPSP_TO_A : public AProblem
{
public:
    explicit CMSRCPSP_TO_A(CScheduler& scheduler);
    ~CMSRCPSP_TO_A() override = default;

	SProblemEncoding& GetProblemEncoding() override;
	void Evaluate(AIndividual& individual) override;
    void LogSolution(AIndividual& individual) override;
    void LogAdditionalData() override {};
private:
    void CreateProblemEncoding();

    std::vector<std::vector<TResourceID>> m_CapableResources;
    SProblemEncoding m_ProblemEncoding;
    std::vector<float> m_MaxObjectiveValues;
    std::vector<float> m_MinObjectiveValues;

    CScheduler m_Scheduler;
};