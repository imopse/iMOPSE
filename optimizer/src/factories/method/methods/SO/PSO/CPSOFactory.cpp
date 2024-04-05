#include "CPSOFactory.h"
#include <stdexcept>
#include "../../../../../utils/fileReader/CReadUtils.h"
#include "../../../../../method/methods/SO/PSO/CPSO.h"

std::vector<float>* CPSOFactory::objectiveWeights = nullptr;

CPSO* CPSOFactory::CreatePSO(SConfigMap* configMap, AProblem& problem, AInitialization* initialization)
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

    return new CPSO(
        *objectiveWeights,
        problem,
        *initialization,
        configMap
    );
}

void CPSOFactory::DeleteObjects()
{
    delete objectiveWeights;
}
