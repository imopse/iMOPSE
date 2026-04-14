
#pragma once

#include "../../AMethod.h"
#include "utils/logger/CExperimentLogger.h"
#include "method/methods/SO/utils/experiment/CSOExperimentUtils.h"

class ASOMethod : public AMethod
{
protected:
    void LogResultData(SSOIndividual& best, AProblem& problem)
    {
        CExperimentLogger::LogData();

        std::string resultString = CSOExperimentUtils::BestToCSVString(best);
        CExperimentLogger::LogResult(resultString.c_str());
        problem.LogSolution(best);
    }
};
