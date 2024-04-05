#pragma once

#include "../../../../../problem/AProblem.h"
#include "../../../../../method/methods/MO/NTGA2/CNTGA2.h"

class CNTGA2Factory
{
public:
    static CNTGA2 *CreateNTGA2(SConfigMap *configMap, AProblem &problem, AInitialization *initialization,
                               ACrossover *crossover,
                               AMutation *mutation);
    static void DeleteObjects();
private:
    static CRankedTournament *rankedTournament;
    static CGapSelectionByRandomDim *gapSelectionByRandomDim;
};
