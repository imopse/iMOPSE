#include "CBNTGPFactory.h"

#include "../../../../../method/methods/MO/BNTGP/CBNTGP.h"

#include <stdexcept>

AMethod* CBNTGPFactory::CreateBNTGP(
    SConfigMap* const configuration,
    AProblem& problem,
    AInitialization* const initialization)
{
    if (initialization == nullptr)
    {
        throw std::runtime_error(
            "BNTGP requires a valid initialization object."
        );
    }

    return new CBNTGP(
        problem,
        *initialization,
        configuration
    );
}