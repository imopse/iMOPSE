#include <cstring>
#include "CMultiMutationFactory.h"
#include "CMutationFactory.h"
#include "method/multiOperator/operatorSelectors/CUniformMultiOperator.h"
#include "method/multiOperator/operatorSelectors/CCreditRouletteMultiOperator.h"
#include "method/multiOperator/operatorSelectors/CInvCreditRouletteMultiOperator.h"
#include "utils/fileReader/CReadUtils.h"

AMultiOperator<AMutation>* CMultiMutationFactory::Create(SConfigMap* configMap, AProblem& problem)
{
    // TODO - implement other options

    std::string rawString;
    if (!configMap->TakeValue("MultiMutation", rawString))
    {
        return nullptr;
    }

    auto const vec = CReadUtils::SplitLine(rawString);
    const char *opName = vec[0].c_str();

    AMultiOperator<AMutation>* multiOperator = nullptr;
    if (strcmp(opName, "UniformMultiOperator") == 0)
    {
        multiOperator = new CUniformMultiOperator<AMutation>();
    }
    else if (strcmp(opName, "CreditRouletteMultiOperator") == 0)
    {
        multiOperator = new CCreditRouletteMultiOperator<AMutation>();
    }
    else if (strcmp(opName, "InvCreditRouletteMultiOperator") == 0)
    {
        multiOperator = new CInvCreditRouletteMultiOperator<AMutation>();
    }
    
    if (multiOperator)
    {
        // TODO - we might think of better way of finding sub operators
        const std::string subMutationKey("SubMutation");
        int subOperatorIdx = 0;
        std::string nextSubMutationKey = subMutationKey + std::to_string(subOperatorIdx);
        while (configMap->HasValue(nextSubMutationKey))
        {
            if (AMutation* atomicOperator = CMutationFactory::Create(configMap, nextSubMutationKey, problem))
            {
                multiOperator->AddOperator(atomicOperator);
            }
            nextSubMutationKey = subMutationKey + std::to_string(++subOperatorIdx);
        }
    }

    return multiOperator;
}
