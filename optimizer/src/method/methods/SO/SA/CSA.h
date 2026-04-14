#pragma  once

#include "../../../configMap/SConfigMap.h"
#include "../../../individual/SO/SSOIndividual.h"
#include "method/methods/SO/ASOMethod.h"

class CSA : public ASOMethod
{
public:
    CSA( AProblem* evaluator, AInitialization* initialization, SConfigMap* , std::vector<float>* objectiveWeights);
    ~CSA() {
        delete m_Problem;
        delete m_Initialization;
        delete m_ObjectiveWeights;
    };

    void RunOptimization() override;

    void Reset() override { delete m_CurrentSolution; }

private:
    SSOIndividual* m_CurrentSolution;
    AProblem* m_Problem;
    AInitialization* m_Initialization;
    std::vector<float>* m_ObjectiveWeights;

    double m_InitialTemperature;
    double m_FinalTemperature;
    double m_CoolingRate;

    void InitializeSolution();
    void Iterate(double temperature);
};

