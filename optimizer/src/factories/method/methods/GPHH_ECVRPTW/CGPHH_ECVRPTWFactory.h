#pragma once

#include "method/configMap/SConfigMap.h"
#include "problem/AProblem.h"
#include "method/AMethod.h"
#include "method/methods/SO/GPHH/operators/CGPHHInitialization.h"
#include "method/methods/SO/GPHH/operators/CGPHHCrossover.h"
#include "method/methods/SO/GPHH/operators/CGPHHMutation.h"
#include "method/methods/SO/GPHH/constructive/IGPHHConstructive.h"

class CGPHH_ECVRPTWFactory {
public:
    static AMethod* CreateGPHH(
        SConfigMap* configMap,
        AProblem* problem
    );

private:
    static std::vector<std::string> Split(const std::string& s, char delimiter);
};
