
#include <stdexcept>
#include "CInitializationFactory.h"
#include "method/operators/initialization/initializations/CInitialization.h"
#include "method/operators/initialization/initializations/CECVRPTWInitialization.h"

AInitialization *CInitializationFactory::Create(SConfigMap* configMap, AProblem& problem)
{
    std::string initializationName;

    if (!configMap->TakeValue("InitializationName", initializationName)) {
        return new CInitialization();
    }

    if (strcmp(initializationName.c_str(), "ECVRPTW") == 0)
        return new CECVRPTWInitialization(dynamic_cast<CECVRPTW&>(problem));

    throw std::runtime_error("Initialization name: " + std::string(initializationName) + " not supported");
}
