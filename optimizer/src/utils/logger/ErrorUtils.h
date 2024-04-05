#pragma once

#include <string>

class ErrorUtils
{
public:
    static void LowerThanZeroI(std::string objectName, std::string paramName, int paramValue);
    static void LowerThanZeroF(std::string objectName, std::string paramName, float paramValue);
    static void OutOfScopeF(std::string objectName, std::string paramName, float paramValue);
};