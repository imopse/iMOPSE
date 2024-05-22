
#include "CNTGA2_ALNSFactory.h"
#include "../../../operators/initialization/CInitializationFactory.h"
#include "../../../operators/selection/CSelectionFactory.h"
#include "../../../operators/mutation/CMutationFactory.h"
#include "../../../operators/crossover/CCrossoverFactory.h"

CRankedTournament * CNTGA2_ALNSFactory::rankedTournament = nullptr;
CRankedTournament* CNTGA2_ALNSFactory::alnsRankedTournament = nullptr;
CGapSelectionByRandomDim * CNTGA2_ALNSFactory::gapSelectionByRandomDim = nullptr;
std::vector<CALNS*>* CNTGA2_ALNSFactory::s_alnsInstances;

CNTGA2_ALNS * CNTGA2_ALNSFactory::CreateNTGA2_ALNS(SConfigMap *configMap
    , AProblem &problem
    , AInitialization *initialization
    , ACrossover *crossover
    , AMutation *mutation
    , std::vector<CALNS*>* alnsInstances
)
{

    rankedTournament = CSelectionFactory::CreateRankedTournamentSelection(configMap);
    gapSelectionByRandomDim = CSelectionFactory::CreateGapSelection(configMap, false);
    s_alnsInstances = alnsInstances;
    std::string selector = "ALNSTournament";
    alnsRankedTournament = CSelectionFactory::CreateRankedTournamentSelection(configMap, selector);

    return new CNTGA2_ALNS(
            problem,
            *initialization,
            *rankedTournament,
            *alnsRankedTournament,
            *gapSelectionByRandomDim,
            *crossover,
            *mutation,
            configMap,
            *s_alnsInstances
    );
}

void CNTGA2_ALNSFactory::DeleteObjects()
{
    delete rankedTournament;
    delete gapSelectionByRandomDim;
    if (s_alnsInstances != nullptr) {
        for (int i = 0; i < s_alnsInstances->size(); i++) {
            delete (*s_alnsInstances)[i];
        }
    }
    delete s_alnsInstances;
}
