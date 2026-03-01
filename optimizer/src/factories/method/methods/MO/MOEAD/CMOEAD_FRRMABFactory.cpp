#include "CMOEAD_FRRMABFactory.h"
#include "../../../operators/mutation/CMutationFactory.h"
#include "../../../operators/crossover/CCrossoverFactory.h"
#include "../../../operators/selection/CSelectionFactory.h"
#include "../../../operators/initialization/CInitializationFactory.h"


CMOEAD_FRRMAB* CMOEAD_FRRMABFactory::CreateMOEAD_FRRMAB(SConfigMap* configMap, AProblem& problem, AInitialization* initialization,
                                                 ACrossover* crossover, AMutation* mutation)
{
    return new CMOEAD_FRRMAB(
        problem,
        *initialization,
        *crossover,
        *mutation,
        configMap
    );
}

void CMOEAD_FRRMABFactory::DeleteObjects()
{

}
