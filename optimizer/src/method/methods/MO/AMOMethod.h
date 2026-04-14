
#pragma once

#include <sstream>
#include "../../AMethod.h"
#include "utils/dataStructures/CCSV.h"
#include "method/methods/MO/utils/archive/ArchiveUtils.h"
#include "utils/logger/CExperimentLogger.h"

class AMOMethod : public AMethod
{
protected:
    void LogParetoFront(const std::vector<SMOIndividual*>& archive)
    {
        std::ostringstream oss;
        CCSV<float>::ToCSV(oss, ArchiveUtils::ToEvaluation(archive));
        CExperimentLogger::LogResult(oss.str().c_str());
    }
};
