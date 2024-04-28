#pragma once

#include "../../../../../problem/AProblem.h"
#include "../../../../../method/methods/MO/NTGA2_ALNS/CNTGA2_ALNS.h"

class CNTGA2_ALNSFactory
{
public:
    static CNTGA2_ALNS *CreateNTGA2_ALNS(SConfigMap *configMap
        , AProblem &problem
        , AInitialization *initialization
        , ACrossover *crossover
        , AMutation *mutation
        , std::vector<AMutation*>* alnsRemovalMutations
        , std::vector<AMutation*>* alnsInsertionMutations);
    static void DeleteObjects();
private:
    static CRankedTournament *rankedTournament;
    static CGapSelectionByRandomDim *gapSelectionByRandomDim;
    static std::vector<AMutation*>* s_alnsRemovalMutations;
    static std::vector<AMutation*>* s_alnsInsertionMutations;
};
