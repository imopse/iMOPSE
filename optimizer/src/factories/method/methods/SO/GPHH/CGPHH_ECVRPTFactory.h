#pragma once

//#include "../../../configMap/SConfigMap.h"
#include "../../../../../method/configMap/SConfigMap.h"
#include "../../../../../problem/AProblem.h"
#include "../../../../../method/AMethod.h"
#include "../../../../../method/methods/SO/GPHH/operators/CGPHHInitialization.h"
#include "../../../../../method/methods/SO/GPHH/operators/CGPHHCrossover.h"
#include "../../../../../method/methods/SO/GPHH/operators/CGPHHMutation.h"
#include "../../../../../method/methods/SO/GPHH/constructive/IGPHHConstructive.h"

class CGPHH_ECVRPTFactory {
public:
    static AMethod* CreateGPHH(
        SConfigMap* configMap,
        AProblem& problem
    );
    
    static void DeleteObjects();

private:
    static CGPHHInitialization* initialization;
    static CGPHHCrossover* crossover;
    static CGPHHMutation* mutation;
    static IGPHHConstructive* constructive;
    
    static std::vector<std::string> Split(const std::string& s, char delimiter);
};
