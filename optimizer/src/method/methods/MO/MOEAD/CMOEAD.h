#pragma once

#include <chrono>
#include <set>
#include "../AMOGeneticMethod.h"
#include "../../../configMap/SConfigMap.h"

class CMOEAD : public AMOGeneticMethod
{
public:
    CMOEAD(
            AProblem &evaluator,
            AInitialization &initialization,
            ACrossover &crossover,
            AMutation &mutation,
            SConfigMap *configMap
    );
    ~CMOEAD() override = default;

    void RunOptimization() override;
protected:
    struct SSubproblem
    {
        std::vector<float> m_WeightVector;
        std::vector<size_t> m_Neighborhood;
    };
    
    size_t m_PartitionsNumber = 0;
    size_t m_NeighbourhoodSize = 0;
    size_t m_GenerationLimit = 0;
    std::vector<SSubproblem> m_Subproblems;
    
    void EvolveToNextGeneration();
    void ConstructSubproblems(size_t number, size_t size);
    void ConstructSubproblemsSimple2D(size_t number, size_t size);
    void ConstructSubproblemsMultiD(size_t number, size_t size, size_t count);
    bool IsBetterInSubproblem(SMOIndividual* newIndividual, SMOIndividual* oldIndividual, SSubproblem& subproblem);

    // TODO - copied from ParetoAnalyzer - remove it or move somewhere else
    // Calculate Hyper-volume using values as they are (either absolute or normalized), using the reference point
    float CalcHV(std::vector<SMOIndividual*>& individuals, const std::vector<float>& refPoint);

    std::chrono::time_point<std::chrono::steady_clock> m_StartTime;
};