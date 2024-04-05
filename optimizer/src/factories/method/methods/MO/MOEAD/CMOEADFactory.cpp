
#include "CMOEADFactory.h"
#include "../../../operators/mutation/CMutationFactory.h"
#include "../../../operators/crossover/CCrossoverFactory.h"
#include "../../../operators/selection/CSelectionFactory.h"
#include "../../../operators/initialization/CInitializationFactory.h"


CMOEAD *CMOEADFactory::CreateMOEAD(SConfigMap *configMap, AProblem &problem, AInitialization *initialization, ACrossover *crossover, AMutation *mutation)
{
    return new CMOEAD(
            problem,
            *initialization,
            *crossover,
            *mutation,
            configMap
    );
}

void CMOEADFactory::DeleteObjects()
{

}
