#pragma once

#include <fstream>
#include <vector>

#include "problem/problems/SDVRP/CSDVRP.h"

class CSDVRPFactory {
public:
    static CSDVRP *CreateSDVRP(const char *problemDefinitionPath);

    static void DeleteObjects();

private:
    static CSDVRPTemplate *sdvrpTemplate;

    static CSDVRPTemplate *ReadSDVRPTemplate(const char *problemDefinitionPath);
};
