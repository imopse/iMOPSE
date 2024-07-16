
#include "CInitializationFactory.h"
#include "../../../../method/operators/initialization/initializations/CInitialization.h"
#include "../../../../method/operators/initialization/initializations/CECVRPTWInitialization.h"

AInitialization *CInitializationFactory::Create(SConfigMap *configMap)
{
    return new CInitialization();
}

AInitialization* CInitializationFactory::CreateECVRPTW(SConfigMap* configMap, AProblem& problem)
{
    return new CECVRPTWInitialization((CECVRPTW&)problem);
}
