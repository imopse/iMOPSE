#include <stdexcept>
#include "CALNSFactory.h"
#include "../../../../../utils/fileReader/CReadUtils.h"
#include "../../../operators/mutation/CALNSMutationFactory.h"

std::vector<float>* CALNSFactory::objectiveWeights = nullptr;
std::vector<AMutation*>* CALNSFactory::s_removalOperators = nullptr;
std::vector<AMutation*>* CALNSFactory::s_insertionOperators = nullptr;

CALNS* CALNSFactory::CreateALNS(SConfigMap* configMap, AProblem& problem, AInitialization* initialization, int objectiveIndex, bool logProgress)
{
    objectiveWeights = new std::vector<float>();
    *objectiveWeights = { 1.0f };

    if (objectiveIndex == -1) {
        configMap->TakeValue("ObjectiveIndex", objectiveIndex);
    }

    s_removalOperators = CALNSMutationFactory::CreateRemovalOperators(problem, objectiveIndex);
    s_insertionOperators = CALNSMutationFactory::CreateInsertionOperators(problem, objectiveIndex);

    return new CALNS(
        *objectiveWeights,
        problem,
        *initialization,
        configMap,
        objectiveIndex,
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
