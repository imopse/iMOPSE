
#include "CInitializationFactory.h"
#include "../../../../method/operators/initialization/initializations/CInitialization.h"

AInitialization *CInitializationFactory::Create(SConfigMap *configMap)
{
    return new CInitialization();
}
