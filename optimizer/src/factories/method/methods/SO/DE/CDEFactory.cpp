#include <stdexcept>
#include "CDEFactory.h"
#include "../../../../../utils/fileReader/CReadUtils.h"

std::vector<float>* CDEFactory::objectiveWeights = nullptr;

CDE* CDEFactory::CreateDE(SConfigMap* configMap, AProblem& problem, AInitialization* initialization)
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

    return new CDE(
            *objectiveWeights,
            problem,
            *initialization,
            configMap
    );
}

void CDEFactory::DeleteObjects()
{
    delete objectiveWeights;
}
