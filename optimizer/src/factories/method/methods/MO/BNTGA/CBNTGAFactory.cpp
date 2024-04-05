#include "CBNTGAFactory.h"
#include "../../../operators/selection/CSelectionFactory.h"

CGapSelectionByRandomDim *CBNTGAFactory::gapSelectionByRandomDim = nullptr;

CBNTGA *CBNTGAFactory::CreateBNTGA(SConfigMap *configMap, AProblem &problem, AInitialization *initialization,
                                   ACrossover *crossover, AMutation *mutation)
{
    gapSelectionByRandomDim = CSelectionFactory::CreateGapSelection(configMap, true);
    
    return new CBNTGA(
            problem,
            *initialization,
            *crossover,
            *mutation,
            *gapSelectionByRandomDim,
            configMap
    );
}

void CBNTGAFactory::DeleteObjects()
{
    delete gapSelectionByRandomDim;
}
