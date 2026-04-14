#pragma once

#include "../../../configMap/SConfigMap.h"
#include "method/methods/MO/AMOMethod.h"

class CSPEA2 : public AMOMethod
{
public:
    CSPEA2(
            AProblem* evaluator,
            AInitialization* initialization,
            ACrossover* crossover,
            AMutation* mutation,
            SConfigMap *configMap
    );
    
    ~CSPEA2() {
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
private:
    AProblem* m_Problem;
    AInitialization* m_Initialization;
    ACrossover* m_Crossover;
    AMutation* m_Mutation;

    std::vector<SMOIndividual*> m_Population;
    std::vector<SMOIndividual*> m_NextPopulation;
    std::vector<SMOIndividual*> m_Archive;
    
    using TNeighborhood = std::vector<std::pair<size_t, float>>;

    size_t m_PopulationSize = 0;
    size_t m_GenerationLimit = 0;
    size_t m_ArchiveSize = 0;
    
    void EvolveToNextGeneration();
    void BuildNeighborhood(std::vector<SMOIndividual *>& individuals, std::vector<TNeighborhood>& neighborhood);
    float CalcDist(const SMOIndividual &leftInd, const SMOIndividual &rightInd);
    void UpdateDensity(std::vector<SMOIndividual *> &individuals, const std::vector<TNeighborhood> &neighborhood);
    void EnviroSelection(std::vector<SMOIndividual *> &individuals);
    void SplitByDomination(std::vector<SMOIndividual *> &individuals, std::vector<const SMOIndividual *> &dominatedIndividuals, std::vector<const SMOIndividual *> &nonDominatedIndividuals);
    void TruncateByDistance(std::vector<const SMOIndividual *> &filteredIndividuals, size_t maxSize);
    size_t Spea2TournamentSelection(const std::vector<SMOIndividual *> &population);
    void UpdateFineGrainedFitness(std::vector<SMOIndividual *> &individuals, const std::vector<TNeighborhood> &neighborhood);
    void UpdateRawFitness(std::vector<SMOIndividual *> &individuals);
};