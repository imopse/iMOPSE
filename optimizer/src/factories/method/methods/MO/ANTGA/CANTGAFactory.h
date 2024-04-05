#pragma once

#include "../../../../../problem/AProblem.h"
#include "../../../../../method/configMap/SConfigMap.h"
#include "../../../../../method/methods/MO/ANTGA/CANTGA.h"

class CANTGAFactory
{
public:
    static CANTGA* CreateANTGA(SConfigMap* configMap, AProblem& problem, AInitialization* initialization,
                               ACrossover* crossover,
                               AMutation* mutation);
    static void DeleteObjects();
private:
    static CGapSelectionByRandomDim* gapSelectionByRandomDim;
};
