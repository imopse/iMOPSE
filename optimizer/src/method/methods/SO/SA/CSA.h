#pragma  once

#include "../../../configMap/SConfigMap.h"
#include "../../../individual/SO/SSOIndividual.h"
#include "../ASOMethod.h"

class CSA : public ASOMethod
{
public:
    CSA(std::vector<float> &objectiveWeights,AProblem& evaluator, AInitialization& initialization,
        SConfigMap* configMap);
   ~CSA() override;

    void RunOptimization();

    void Reset() { delete m_CurrentSolution; }

private:
    SSOIndividual* m_CurrentSolution;

    double m_InitialTemperature;
    double m_FinalTemperature;
    double m_CoolingRate;

    void InitializeSolution();
    void Iterate(double temperature);
};

