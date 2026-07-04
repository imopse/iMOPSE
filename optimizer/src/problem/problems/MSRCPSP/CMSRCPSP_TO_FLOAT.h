#pragma once

#include "CResource.h"
#include "CScheduler.h"
#include "../../AProblem.h"
#include "../../../method/individual/SGenotype.h"

class CMSRCPSP_TO_FLOAT : public AProblem
{
public:
    explicit CMSRCPSP_TO_FLOAT(CScheduler* scheduler, size_t objCount);
    ~CMSRCPSP_TO_FLOAT()
    {
        delete m_Scheduler;
    }

    SProblemEncoding& GetProblemEncoding() override;
    void Evaluate(AIndividual& individual) override;
    void LogSolution(AIndividual& individual) override;
    void LogAdditionalData() override {};
    CScheduler* GetScheduler() { return m_Scheduler; }
private:
    void CreateProblemEncoding();

    size_t m_ObjCount;
    std::vector<std::vector<TResourceID>> m_CapableResources;
    SProblemEncoding m_ProblemEncoding;
    std::vector<float> m_MaxObjectiveValues;
    std::vector<float> m_MinObjectiveValues;

    CScheduler* m_Scheduler;
};
