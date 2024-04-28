
#include "CNTGA2_ALNSFactory.h"
#include "../../../operators/initialization/CInitializationFactory.h"
#include "../../../operators/selection/CSelectionFactory.h"
#include "../../../operators/mutation/CMutationFactory.h"
#include "../../../operators/crossover/CCrossoverFactory.h"

CRankedTournament * CNTGA2_ALNSFactory::rankedTournament = nullptr;
CGapSelectionByRandomDim * CNTGA2_ALNSFactory::gapSelectionByRandomDim = nullptr;
std::vector<AMutation*>* CNTGA2_ALNSFactory::s_alnsRemovalMutations;
std::vector<AMutation*>* CNTGA2_ALNSFactory::s_alnsInsertionMutations;

CNTGA2_ALNS * CNTGA2_ALNSFactory::CreateNTGA2_ALNS(SConfigMap *configMap
    , AProblem &problem
    , AInitialization *initialization
    , ACrossover *crossover
    , AMutation *mutation
    , std::vector<AMutation*>* alnsRemovalMutations
    , std::vector<AMutation*>* alnsInsertionMutations
)
{

    rankedTournament = CSelectionFactory::CreateRankedTournamentSelection(configMap);
    gapSelectionByRandomDim = CSelectionFactory::CreateGapSelection(configMap, false);
    s_alnsInsertionMutations = alnsInsertionMutations;
    s_alnsRemovalMutations = alnsRemovalMutations;


    return new CNTGA2_ALNS(
            problem,
            *initialization,
            *rankedTournament,
            *gapSelectionByRandomDim,
            *crossover,
            *mutation,
            configMap,
            *s_alnsRemovalMutations,
            *s_alnsInsertionMutations
    );
}

void CNTGA2_ALNSFactory::DeleteObjects()
{
    delete rankedTournament;
    delete gapSelectionByRandomDim;
    for (int i = 0; i < s_alnsRemovalMutations->size(); i++) {
        delete (*s_alnsRemovalMutations)[i];
    }
    for (int i = 0; i < s_alnsInsertionMutations->size(); i++) {
        delete (*s_alnsInsertionMutations)[i];
    }
    delete s_alnsInsertionMutations;
    delete s_alnsRemovalMutations;
}
