#pragma once

#include "../AMOGeneticMethod.h"
#include "../../../configMap/SConfigMap.h"

class CSPEA2 : public AMOGeneticMethod
{
public:
    CSPEA2(
            AProblem &evaluator,
            AInitialization &initialization,
            ACrossover &crossover,
            AMutation &mutation,
            SConfigMap *configMap
    );
    ~CSPEA2() override = default;

    void RunOptimization() override;
private:
    using TNeighborhood = std::vector<std::pair<size_t, float>>;
    
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