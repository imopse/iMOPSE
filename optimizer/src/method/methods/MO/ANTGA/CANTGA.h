#pragma once

#include "method/configMap/SConfigMap.h"
#include "method/individual/MO/SMOIndividual.h"
#include "method/methods/MO/AMOGeneticMethod.h"
#include "method/operators/selection/selections/CRankedTournament.h"
#include "method/operators/selection/selections/CGapSelectionByRandomDim.h"

template <typename T> class CCSV;

class CANTGA : public AMOGeneticMethod
{
public:
    CANTGA(
        AProblem& evaluator,
        AInitialization& initialization,
        ACrossover& crossover,
        AMutation& mutation,
        CGapSelectionByRandomDim& gapSelection,
        SConfigMap* configMap
    );

    ~CANTGA();

    void RunOptimization() override;
    void Reset() override { m_Generation = 0; }

private:
    int m_Generation = 0;
    CGapSelectionByRandomDim& m_GapSelection;
    
    void EvolveToNextGeneration();
    void CrossoverAndMutate(SMOIndividual* firstParent, SMOIndividual* secondParent);
    void LogIndividualsToCSV(CCSV<float>& csv, const std::vector<SMOIndividual*>& individuals) const;
};