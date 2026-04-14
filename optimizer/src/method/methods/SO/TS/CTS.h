#pragma once

#include <list>
#include <memory>
#include "../../../configMap/SConfigMap.h"
#include "../../../individual/SO/SSOIndividual.h"
#include "method/methods/SO/ASOMethod.h"

class CTS : public ASOMethod
{
public:
    CTS( AProblem* evaluator, AInitialization* initialization, SConfigMap* configMap, std::vector<float>* objectiveWeights);
    ~CTS() {
        delete m_Problem;
        delete m_Initialization;
        delete m_ObjectiveWeights;
    }

    void RunOptimization() override;

    void Reset() override
    {
        m_TabuList.clear();
    };

private:
    AProblem* m_Problem;
    AInitialization* m_Initialization;
    std::vector<float>* m_ObjectiveWeights;
    
    int m_TabuListSize;
    int m_MaxIterations;
    float m_SimilarityThreshold;
    std::shared_ptr<SSOIndividual> m_CurrentSolution;
    std::list<std::shared_ptr<SSOIndividual>> m_TabuList;
    
    void InitializeSolution();
    bool IsTabu(const std::shared_ptr<SSOIndividual> candidate);
    void UpdateTabuList(std::shared_ptr<SSOIndividual> newSolution);
};