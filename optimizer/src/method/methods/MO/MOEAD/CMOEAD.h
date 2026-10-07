#pragma once

#include <chrono>
#include <set>
#include "../../../configMap/SConfigMap.h"
#include "method/methods/MO/AMOMethod.h"
#include <cstddef>

struct SMOEADSubproblem
{
    std::vector<float> m_WeightVector;
    std::vector<std::size_t> m_Neighborhood;
};

class CMOEAD : public AMOMethod
{
public:
    CMOEAD(
            AProblem* evaluator,
            AInitialization* initialization,
            ACrossover* crossover,
            AMutation* mutation,
            SConfigMap* configMap
    );
    
    ~CMOEAD() {
        delete m_Problem;
        delete m_Initialization;
        delete m_Crossover;
        delete m_Mutation;
    };

    void RunOptimization() override;

    void Reset() override {
        for (auto& i : m_Population)
        {
            delete i;
        }
        m_Population.clear();
        for (auto &i: m_NextPopulation)
        {
            delete i;
        }
        m_NextPopulation.clear();
        for (auto &i: m_Archive)
        {
            delete i;
        }
        m_Archive.clear();
    }
    
protected:
    AProblem* m_Problem;
    AInitialization* m_Initialization;
    ACrossover* m_Crossover;
    AMutation* m_Mutation;

    std::vector<SMOIndividual*> m_Population;
    std::vector<SMOIndividual*> m_NextPopulation;
    std::vector<SMOIndividual*> m_Archive;

    std::size_t m_PopulationSize = 0;
    std::size_t m_GenerationLimit = 0;
    std::size_t m_PartitionsNumber = 0;
    std::size_t m_NeighbourhoodSize = 0;
    std::vector<SMOEADSubproblem> m_Subproblems;

    void ConstructSubproblems(std::size_t number, std::size_t size);
    
private:
    void EvolveToNextGeneration();
    void ConstructSubproblemsSimple2D(std::size_t number, std::size_t size);
    void ConstructSubproblemsMultiD(std::size_t number, std::size_t size, std::size_t count);
    bool IsBetterInSubproblem(SMOIndividual* newIndividual, SMOIndividual* oldIndividual, SMOEADSubproblem& subproblem);

    // TODO - copied from ParetoAnalyzer - remove it or move somewhere else
    // Calculate Hyper-volume using values as they are (either absolute or normalized), using the reference point
    float CalcHV(std::vector<SMOIndividual*>& individuals, const std::vector<float>& refPoint);

    std::chrono::time_point<std::chrono::steady_clock> m_StartTime;
};
