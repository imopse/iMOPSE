#include "CMethodFactory.h"
#include "configMap/CConfigFactory.h"
#include "methods/SO/GA/CGAFactory.h"
#include "methods/SO/SA/CSAFactory.h"
#include "methods/SO/TS/CTSFactory.h"
#include "methods/MO/NTGA2/CNTGA2Factory.h"
#include "methods/MO/NSGAII/CNSGAIIFactory.h"
#include "methods/SO/ACO/CACOFactory.h"
#include "operators/initialization/CInitializationFactory.h"
#include "operators/crossover/CCrossoverFactory.h"
#include "operators/mutation/CMutationFactory.h"
#include "methods/SO/PSO/CPSOFactory.h"
#include <cstring>
#include "methods/SO/DE/CDEFactory.h"
#include "methods/MO/MOEAD/CMOEADFactory.h"
#include "methods/MO/ANTGA/CANTGAFactory.h"
#include "methods/MO/BNTGA/CBNTGAFactory.h"
#include "methods/MO/SPEA2/CSPEA2Factory.h"
#include "../../utils/fileReader/CReadUtils.h"

// Static members of CMethodFactory, initialized to nullptr. These will hold various components of an optimization method.
SConfigMap* CMethodFactory::configMap = nullptr;
AInitialization* CMethodFactory::initialization = nullptr;
ACrossover* CMethodFactory::crossover = nullptr;
AMutation* CMethodFactory::mutation = nullptr;

// Static method to create an optimization method based on a configuration file and a problem instance.
AMethod* CMethodFactory::CreateMethod(
        const char* optimizerConfigPath,
        AProblem& problem
)
{
    // Create a configuration map from the provided path using the CConfigFactory.
    configMap = CConfigFactory::CreateConfigMap(optimizerConfigPath);
    if (configMap == nullptr) {
        throw std::runtime_error("Error while reading method configuration");
    }

    std::string methodName;
    // Extract the method name from the configuration map. If it's not provided, throw an error.
    if (!configMap->TakeValue("MethodName", methodName)) {
        throw std::runtime_error("MethodName not provided in method configuration");
    }

    // Create initialization strategy based on the configuration map.
    initialization = CInitializationFactory::Create(configMap);
    // TODO - we should not assume that each methods uses mutation/crossover (or exactly one)
    crossover = CCrossoverFactory::Create(configMap, "Crossover", problem);
    if (crossover == nullptr) {
        throw std::runtime_error("Error while reading crossover configuration");
    }
    mutation = CMutationFactory::Create(configMap, "Mutation", problem);
    if (mutation == nullptr) {
        throw std::runtime_error("Error while reading mutation configuration");
    }

    // Create and return a specific optimization method based on the method name.
    if (strcmp(methodName.c_str(), "ACO") == 0)
        return CACOFactory::CreateACO(configMap, problem, initialization, optimizerConfigPath);
    if (strcmp(methodName.c_str(), "SA") == 0)
        return CSAFactory::CreateSA(configMap, problem, initialization);
    if (strcmp(methodName.c_str(), "TS") == 0)
        return CTSFactory::CreateTS(configMap, problem, initialization);
    if (strcmp(methodName.c_str(), "DE") == 0)
        return CDEFactory::CreateDE(configMap, problem, initialization);
    if (strcmp(methodName.c_str(), "PSO") == 0)
        return CPSOFactory::CreatePSO(configMap, problem, initialization);
    
    if (strcmp(methodName.c_str(), "GA") == 0)
        return CGAFactory::CreateGA(configMap, problem, initialization, crossover, mutation);
    if (strcmp(methodName.c_str(), "NTGA2") == 0)
        return CNTGA2Factory::CreateNTGA2(configMap, problem, initialization, crossover, mutation);
    if (strcmp(methodName.c_str(), "NSGAII") == 0)
        return CNSGAIIFactory::CreateNSGAII(configMap, problem, initialization, crossover, mutation);
    if (strcmp(methodName.c_str(), "MOEAD") == 0)
        return CMOEADFactory::CreateMOEAD(configMap, problem, initialization, crossover, mutation);
    if (strcmp(methodName.c_str(), "ANTGA") == 0)
        return CANTGAFactory::CreateANTGA(configMap, problem, initialization, crossover, mutation);
    if (strcmp(methodName.c_str(), "BNTGA") == 0)
        return CBNTGAFactory::CreateBNTGA(configMap, problem, initialization, crossover, mutation);
    if (strcmp(methodName.c_str(), "SPEA2") == 0)
        return CSPEA2Factory::CreateSPEA2(configMap, problem, initialization, crossover, mutation);
    
    // If the method name is not supported, throw an error.
    throw std::runtime_error("Method name: " + std::string(methodName) + " not supported");
}

// Static method to delete the objects created by the factory.
void CMethodFactory::DeleteObjects() {
    // Delete the created objects (configMap, initialization, crossover, mutation)
    delete configMap;
    delete initialization;
    if (crossover != nullptr)
        delete crossover;
    if (mutation != nullptr)
        delete mutation;

    // Call the DeleteObjects methods of other factories involved in creating the method components.
    CGAFactory::DeleteObjects();
    CSAFactory::DeleteObjects();
    CTSFactory::DeleteObjects();
    CNTGA2Factory::DeleteObjects();
    CNSGAIIFactory::DeleteObjects();
    CPSOFactory::DeleteObjects();
    CMOEADFactory::DeleteObjects();
    CBNTGAFactory::DeleteObjects();
    CSPEA2Factory::DeleteObjects();
    CACOFactory::DeleteObjects();
}
