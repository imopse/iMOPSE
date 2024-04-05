#include "CSPEA2Factory.h"

CSPEA2 *CSPEA2Factory::CreateSPEA2(SConfigMap *configMap, AProblem &problem, AInitialization *initialization,
                                   ACrossover *crossover,
                                   AMutation *mutation)
{
    return new CSPEA2(
            problem,
            *initialization,
            *crossover,
            *mutation,
            configMap
    );
}

void CSPEA2Factory::DeleteObjects()
{

}