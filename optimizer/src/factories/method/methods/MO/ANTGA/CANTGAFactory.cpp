#include "CANTGAFactory.h"
#include "../../../operators/selection/CSelectionFactory.h"

CGapSelectionByRandomDim* CANTGAFactory::gapSelectionByRandomDim = nullptr;

CANTGA* CANTGAFactory::CreateANTGA(SConfigMap* configMap, AProblem& problem, AInitialization* initialization,
                                   ACrossover* crossover, AMutation* mutation)
{
    gapSelectionByRandomDim = CSelectionFactory::CreateGapSelection(configMap, true);

    return new CANTGA(
        problem,
        *initialization,
        *crossover,
        *mutation,
        *gapSelectionByRandomDim,
        configMap
    );
}

void CANTGAFactory::DeleteObjects()
{
    delete gapSelectionByRandomDim;
}
