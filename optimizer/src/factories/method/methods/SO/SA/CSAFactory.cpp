#include <stdexcept>
#include "CSAFactory.h"
#include "../../../../../utils/fileReader/CReadUtils.h"

std::vector<float>* CSAFactory::objectiveWeights = nullptr;

CSA* CSAFactory::CreateSA(SConfigMap* configMap, AProblem& problem, AInitialization* initialization)
{
    objectiveWeights = new std::vector<float>();
    std::string rawWeightsString;
    configMap->TakeValue("ObjectiveWeights", rawWeightsString);
    if (!rawWeightsString.empty())
    {
        CReadUtils::ReadWeights(rawWeightsString, *objectiveWeights);
    } else {
        *objectiveWeights = {1.0f};
    }

    return new CSA(
            *objectiveWeights,
            problem,
            *initialization,
            configMap
    );
}

void CSAFactory::DeleteObjects()
{
    delete objectiveWeights;
}