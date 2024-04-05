#pragma once

#include <list>
#include <memory>
#include "../../../configMap/SConfigMap.h"
#include "../../../individual/SO/SSOIndividual.h"
#include "../ASOMethod.h"

class CTS : public ASOMethod
{
public:
    CTS(std::vector<float> &objectiveWeights, AProblem& evaluator, AInitialization& initialization,
        SConfigMap* configMap);
    ~CTS() override = default;

    void RunOptimization() override;

    void Reset()
    {
        m_TabuList.clear();
    };

private:
    int m_TabuListSize;
    int m_MaxIterations;
    float m_SimilarityThreshold;
    std::shared_ptr<SSOIndividual> m_CurrentSolution;
    std::list<std::shared_ptr<SSOIndividual>> m_TabuList;
    
    void InitializeSolution();
    bool IsTabu(const std::shared_ptr<SSOIndividual> candidate);
    void UpdateTabuList(std::shared_ptr<SSOIndividual> newSolution);
};