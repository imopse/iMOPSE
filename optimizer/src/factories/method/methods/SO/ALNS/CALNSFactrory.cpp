#include <stdexcept>
#include "CALNSFactory.h"
#include "../../../../../utils/fileReader/CReadUtils.h"
#include "../../../operators/mutation/CALNSMutationFactory.h"

std::vector<float>* CALNSFactory::objectiveWeights = nullptr;
std::vector<AMutation*>* CALNSFactory::s_removalOperators = nullptr;
std::vector<AMutation*>* CALNSFactory::s_insertionOperators = nullptr;

CALNS* CALNSFactory::CreateALNS(SConfigMap* configMap, AProblem& problem, AInitialization* initialization, bool logProgress, int* objectiveIndex)
{
    objectiveWeights = new std::vector<float>();
    if (objectiveIndex == nullptr) {
        std::string rawWeightsString;
        configMap->TakeValue("ObjectiveWeights", rawWeightsString);
        if (!rawWeightsString.empty())
        {
            CReadUtils::ReadWeights(rawWeightsString, *objectiveWeights);
        }
        else {
            *objectiveWeights = { 1.0f };
        }
    }
    else 
    {
        auto objectives = problem.GetProblemEncoding().m_objectivesNumber;
        objectiveWeights = new std::vector<float>(objectives, 0);
        (*objectiveWeights)[*objectiveIndex] = 1;
    }

    s_removalOperators = CALNSMutationFactory::CreateRemovalOperators(problem, *objectiveWeights);
    s_insertionOperators = CALNSMutationFactory::CreateInsertionOperators(problem, *objectiveWeights);

    return new CALNS(
        *(new std::vector<float>(*objectiveWeights)),
        problem,
        *initialization,
        configMap,
        logProgress,
        *s_removalOperators,
        *s_insertionOperators
    );
}

void CALNSFactory::DeleteObjects()
{
    delete objectiveWeights;
    if (s_removalOperators != nullptr) {
        for (int i = 0; i < s_removalOperators->size(); i++) {
            delete (*s_removalOperators)[i];
        }
    }
    delete s_removalOperators;
    if (s_insertionOperators != nullptr) {
        for (int i = 0; i < s_insertionOperators->size(); i++) {
            delete (*s_insertionOperators)[i];
        }
    }
    delete s_insertionOperators;
}
