
#include <stdexcept>
#include "CGAFactory.h"
#include "../../../../../utils/fileReader/CReadUtils.h"
#include "../../../operators/selection/CSelectionFactory.h"

std::vector<float> *CGAFactory::objectiveWeights = nullptr;
CFitnessTournament *CGAFactory::fitnessTournament = nullptr;

CGA *CGAFactory::CreateGA(SConfigMap *configMap, AProblem &problem, AInitialization *initialization,
                          ACrossover *crossover,
                          AMutation *mutation)
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
    
    fitnessTournament = CSelectionFactory::CreateFitnessTournamentSelection(configMap);

    return new CGA(
            *objectiveWeights,
            problem,
            *initialization,
            *fitnessTournament,
            *crossover,
            *mutation,
            configMap
    );
}

void CGAFactory::DeleteObjects()
{
    delete objectiveWeights;
    delete fitnessTournament;
}
