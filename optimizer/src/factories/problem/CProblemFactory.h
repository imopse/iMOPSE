#pragma once

#include "../../problem/AProblem.h"

class CProblemFactory
{
public:
    static AProblem *CreateProblem(const char *problemName, const char *problemConfigurationPath);
    static void DeleteObjects();
};