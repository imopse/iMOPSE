#pragma once

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
    
    void Reset() override {
        m_Generation = 0;
    }
private:
    int m_Generation = 0;
    CGapSelectionByRandomDim &m_GapSelection;
    
    void EvolveToNextGeneration();
    void CrossoverAndMutate(SMOIndividual *firstParent, SMOIndividual *secondParent);
};
