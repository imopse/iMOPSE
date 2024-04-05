
#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include "../../../method/configMap/SConfigMap.h"

class CConfigFactory
{
public:
    static SConfigMap *CreateConfigMap(const char *path);
};
