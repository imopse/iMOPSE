#pragma once

#include <vector>
#include <string>
#include "../../../../individual/MO/SMOIndividual.h"

namespace ArchiveUtils
{
    void CopyToArchiveWithFiltering(const std::vector<SMOIndividual *> &individuals, std::vector<SMOIndividual *> &archive);
    void CopyToArchiveForSingleObjective(const std::vector<SMOIndividual *> &individuals,
                                         std::vector<SMOIndividual *> &archive, std::vector<float> objectiveWeights);
    void CopyToArchiveWithFiltering(const SMOIndividual *individual, std::vector<SMOIndividual *> &archive);
    void SaveArchiveToFile(const std::vector<SMOIndividual *> &archive);
    void SaveArchiveToFile(const std::vector<SMOIndividual *> &archive, const std::string &instanceName, int configId,
                           int runId);
    std::vector<std::vector<float>> ToEvaluation(const std::vector<SMOIndividual *> &archive);
    void LogParetoFront(const std::vector<SMOIndividual *> &archive);
};
