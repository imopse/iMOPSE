
#include "CNSGAIIFactory.h"
#include "../../../operators/mutation/CMutationFactory.h"
#include "../../../operators/crossover/CCrossoverFactory.h"
#include "../../../operators/selection/CSelectionFactory.h"
#include "../../../operators/initialization/CInitializationFactory.h"

CRankedTournament *CNSGAIIFactory::rankedTournament = nullptr;

CNSGAII *CNSGAIIFactory::CreateNSGAII(SConfigMap *configMap, AProblem &problem, AInitialization *initialization,
                                      ACrossover *crossover,
                                      AMutation *mutation)
{
    rankedTournament = CSelectionFactory::CreateRankedTournamentSelection(configMap);

    return new CNSGAII(
            problem,
            *initialization,
            *rankedTournament,
            *crossover,
            *mutation,
            configMap
    );
}

void CNSGAIIFactory::DeleteObjects()
{
    delete rankedTournament;
}
