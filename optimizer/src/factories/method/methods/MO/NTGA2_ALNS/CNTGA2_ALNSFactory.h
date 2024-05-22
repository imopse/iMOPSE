#pragma once

#include "../../../../../problem/AProblem.h"
#include "../../../../../method/methods/MO/NTGA2_ALNS/CNTGA2_ALNS.h"
#include "../../../../../method/methods/SO/ALNS/CALNS.h"

class CNTGA2_ALNSFactory
{
public:
    static CNTGA2_ALNS *CreateNTGA2_ALNS(SConfigMap *configMap
        , AProblem &problem
        , AInitialization *initialization
        , ACrossover *crossover
        , AMutation *mutation
        , std::vector<CALNS*>* alnsInstances);
    static void DeleteObjects();
private:
    static CRankedTournament *rankedTournament;
    static CRankedTournament* alnsRankedTournament;
    static CGapSelectionByRandomDim *gapSelectionByRandomDim;
    static std::vector<CALNS*>* s_alnsInstances;
};
