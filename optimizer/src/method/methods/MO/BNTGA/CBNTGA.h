#pragma once

#include <chrono>
#include "../../../configMap/SConfigMap.h"
#include "../../../individual/MO/SMOIndividual.h"
#include "../AMOGeneticMethod.h"
#include "../../../operators/selection/selections/CRankedTournament.h"
#include "../../../operators/selection/selections/CGapSelectionByRandomDim.h"

class CBNTGA : public AMOGeneticMethod
{
public:
    CBNTGA(
            AProblem &evaluator,
            AInitialization &initialization,
            ACrossover &crossover,
            AMutation &mutation,
            CGapSelectionByRandomDim& gapSelection,
            SConfigMap *configMap
    );
    ~CBNTGA() override = default;

    void RunOptimization() override;

private:
    CGapSelectionByRandomDim &m_GapSelection;
    
    void EvolveToNextGeneration();
    void CrossoverAndMutate(SMOIndividual *firstParent, SMOIndividual *secondParent);

    // TODO - copied from ParetoAnalyzer - remove it or move somewhere else
    // Calculate Hyper-volume using values as they are (either absolute or normalized), using the reference point
    float CalcHV(std::vector<SMOIndividual*>& individuals, const std::vector<float>& refPoint);

    std::chrono::time_point<std::chrono::steady_clock> m_StartTime;
};
