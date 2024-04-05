#include <stdexcept>
#include "ErrorUtils.h"

void ErrorUtils::LowerThanZeroI(std::string objectName, std::string paramName, int paramValue)
{
    if (paramValue < 0) throw std::runtime_error(objectName + " parameter " + paramName + " lower than 0");
}

void ErrorUtils::LowerThanZeroF(std::string objectName, std::string paramName, float paramValue)
{
    if (paramValue < 0) throw std::runtime_error(objectName + " parameter " + paramName + " lower than 0");
}

void ErrorUtils::OutOfScopeF(std::string objectName, std::string paramName, float paramValue)
{
    if (paramValue < 0 || paramValue > 1.0) throw std::runtime_error(objectName + " parameter " + paramName + " out of [0,1] scope");
}
