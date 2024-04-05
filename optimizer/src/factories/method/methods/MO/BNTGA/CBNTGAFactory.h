#pragma once

#include "../../../../../problem/AProblem.h"
#include "../../../../../method/methods/MO/NSGAII/CNSGAII.h"
#include "../../../../../method/configMap/SConfigMap.h"
#include "../../../../../method/methods/MO/BNTGA/CBNTGA.h"

class CBNTGAFactory
{
public:
    static CBNTGA *CreateBNTGA(SConfigMap *configMap, AProblem &problem, AInitialization *initialization,
                               ACrossover *crossover,
                               AMutation *mutation);
    static void DeleteObjects();
private:
    static CGapSelectionByRandomDim *gapSelectionByRandomDim;
};
