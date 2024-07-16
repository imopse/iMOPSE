#pragma once

#include "../../../../../method/methods/SO/ALNS/CALNS.h"

class CALNSFactory
{
public:
    static CALNS* CreateALNS(SConfigMap* configMap, AProblem& problem, AInitialization* initialization, bool logProgress, int* objectiveIndex = nullptr);
    static void DeleteObjects();
private:
    static std::vector<float>* objectiveWeights;
    static std::vector<AMutation*>* s_removalOperators;
    static std::vector<AMutation*>* s_insertionOperators;
};
