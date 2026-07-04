#pragma once
#include <vector>
#include <limits>
#include <chrono>
#include "method/methods/SO/ASOMethod.h"
#include "method/configMap/SConfigMap.h"

class AProblem;
class AInitialization;

class CLSHADE : public ASOMethod
{
public:
    CLSHADE(AProblem* evaluator, AInitialization* initialization, SConfigMap* configMap, std::vector<float>* objectiveWeights);
    ~CLSHADE() {
        delete m_Problem;
        delete m_Initialization;
        delete m_ObjectiveWeights;
    };
    
    void Reset() override
    {
        for (auto* ind : m_Population) delete ind;
        m_Population.clear();
        for (auto* ind : m_Archive) delete ind;
        m_Archive.clear();
        m_SF.clear();
        m_SCR.clear();
        m_DeltaFitness.clear();
        m_k = 0;
        m_CurrentFE = 0;
        m_MF.clear();
        m_MCR.clear();
        m_MF.resize(m_CrFMemoryListSize, 0.1);
        m_MCR.resize(m_CrFMemoryListSize, 0.1);
    };
    
    void RunOptimization() override;
private:
    AProblem* m_Problem;
    AInitialization* m_Initialization;
    std::vector<float>* m_ObjectiveWeights;
    std::vector<SSOIndividual*> m_Population;
    std::vector<SSOIndividual*> m_Archive;
    
    float m_offset = 0;
    float bestFitness = 0;
    int m_PopulationSizeInit = 0;
    int m_PopulationSizeMin = 4;
    int m_CrFMemoryListSize = 5;
    int m_MAXFE = 0;
    int m_CurrentFE = 0;
    float m_pbestRate = 0.1f;
    std::vector<float> m_MF;
    std::vector<float> m_MCR;
    size_t m_k = 0;
    std::vector<float> m_SF;
    std::vector<float> m_SCR;
    std::vector<float> m_DeltaFitness;
    
    void CreateInitialPopulation();
    void EvolveOneGeneration();
    void UpdatePopulationSize();
    void UpdateSuccessMemory();
    void EvaluateIndividual(SSOIndividual& ind);
    void DifferentialEvolutionStep(SSOIndividual& donor, const SSOIndividual& target, const SSOIndividual& pbest, const SSOIndividual& r1, const SSOIndividual& r2, float F, float CR);
    void MaintainArchiveSize();
};
