#pragma once

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
private:
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
    bool IsBetterInSubproblem(SMOIndividual *pIndividual, SMOIndividual *&pIndividual1, SSubproblem &subproblem);
};