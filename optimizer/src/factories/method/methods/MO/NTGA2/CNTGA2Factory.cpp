
#include "CNTGA2Factory.h"
#include "../../../operators/initialization/CInitializationFactory.h"
#include "../../../operators/selection/CSelectionFactory.h"
#include "../../../operators/mutation/CMutationFactory.h"
#include "../../../operators/crossover/CCrossoverFactory.h"

CRankedTournament *CNTGA2Factory::rankedTournament = nullptr;
CGapSelectionByRandomDim *CNTGA2Factory::gapSelectionByRandomDim = nullptr;

CNTGA2 *CNTGA2Factory::CreateNTGA2(SConfigMap *configMap, AProblem &problem, AInitialization *initialization,
                                   ACrossover *crossover,
                                   AMutation *mutation)
{

    rankedTournament = CSelectionFactory::CreateRankedTournamentSelection(configMap);
    gapSelectionByRandomDim = CSelectionFactory::CreateGapSelection(configMap, false);

    return new CNTGA2(
            problem,
            *initialization,
            *rankedTournament,
            *gapSelectionByRandomDim,
            *crossover,
            *mutation,
            configMap
    );
}

void CNTGA2Factory::DeleteObjects()
{
    delete rankedTournament;
    delete gapSelectionByRandomDim;
}
