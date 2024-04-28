#pragma once

#include "CResource.h"
#include "CScheduler.h"
#include "../../AProblem.h"
#include "../../../method/individual/SGenotype.h"

class CMSRCPSP_TA : public AProblem
{
public:
    explicit CMSRCPSP_TA(CScheduler &scheduler, size_t objCount);
    ~CMSRCPSP_TA() override = default;

    SProblemEncoding& GetProblemEncoding() override;
    void Evaluate(AIndividual& individual) override;
    void LogSolution(AIndividual& individual) override;
    void LogAdditionalData() override {};

    // MSRCPSP specific functions
    float FindBestGeneValueCostWise(size_t geneIdx) const;
    std::vector<size_t> FindNumberOfResourcesUse(const std::vector<float>& solution) const;
    float FindBestGeneValueUsageWise(size_t geneIdx, const std::vector<size_t>& currentResourcesUsage) const;

private:
    void CreateProblemEncoding();

    size_t m_ObjCount;
    std::vector<std::vector<TResourceID>> m_CapableResources;
    std::vector<size_t> m_UpperBounds;
    SProblemEncoding m_ProblemEncoding;
    std::vector<float> m_MaxObjectiveValues;
    std::vector<float> m_MinObjectiveValues;

    CScheduler m_Scheduler;
};