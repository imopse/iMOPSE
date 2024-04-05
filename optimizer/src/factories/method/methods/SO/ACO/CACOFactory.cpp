#include <stdexcept>
#include "CACOFactory.h"
#include "../../../../../utils/fileReader/CReadUtils.h"
#include <string>

std::vector<float> *CACOFactory::objectiveWeights = nullptr;

CACO *CACOFactory::CreateACO(SConfigMap *configMap, AProblem &problem, AInitialization *initialization,const char *optimizerConfigPath) {
    objectiveWeights = new std::vector<float>();
    std::string rawWeightsString;
    configMap->TakeValue("ObjectiveWeights", rawWeightsString);

    if (!rawWeightsString.empty())
    {
        CReadUtils::ReadWeights(rawWeightsString, *objectiveWeights);
    } else {
        *objectiveWeights = {1.0f};
    }

    std::string strVariable(optimizerConfigPath);
    
    if (strVariable.find("CVRP") != std::string::npos) {
        return new CACO_TSP(
                problem,
                *initialization,
                configMap,
                *objectiveWeights
        );
    }
    throw std::runtime_error("Method ACO not supported for problem " + std::string(optimizerConfigPath) );
}

void CACOFactory::DeleteObjects() {
    delete objectiveWeights;
}
