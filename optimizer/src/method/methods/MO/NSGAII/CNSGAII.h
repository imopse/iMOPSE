#pragma once

#include <vector>
#include "../../../configMap/SConfigMap.h"
#include "../../../individual/MO/SMOIndividual.h"
#include "../AMOGeneticMethod.h"
#include "../../../operators/selection/selections/CRankedTournament.h"

class CNSGAII : public AMOGeneticMethod
{
public:
    CNSGAII(
            AProblem &evaluator,
            AInitialization &initialization,
            CRankedTournament &rankedTournament,
            ACrossover &crossover,
            AMutation &mutation,
            SConfigMap *configMap
    );
    ~CNSGAII() override = default;

    void RunOptimization() override;
private:
    CRankedTournament &m_RankedTournament;

    void EvolveToNextGeneration();
    void CalcCrowdingDistance(std::vector<SMOIndividual *> &population, std::vector<size_t> &indices);
};