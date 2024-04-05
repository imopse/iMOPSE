#include <stdexcept>
#include "CTSFactory.h"
#include "../../../../../utils/fileReader/CReadUtils.h"

std::vector<float>* CTSFactory::objectiveWeights = nullptr;
// Other specific members for CTS

CTS* CTSFactory::CreateTS(SConfigMap* configMap, AProblem& problem, AInitialization* initialization)
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

    // Additional TA-specific configuration handling goes here

    return new CTS(
            *objectiveWeights,
            problem,
            *initialization,
            configMap
    );
}

void CTSFactory::DeleteObjects()
{
    delete objectiveWeights;
    // Cleanup for other specific members
}