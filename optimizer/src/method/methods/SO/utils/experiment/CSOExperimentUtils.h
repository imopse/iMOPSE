#pragma once

#include "../../../../individual/SO/SSOIndividual.h"
#include "../../../../../problem/AProblem.h"
#include "../../../../individual/SO/SParticle.h"
#include <vector>
#include <string>

class CSOExperimentUtils
{
public:
    static void AddExperimentData(int generation, const std::vector<SSOIndividual*>& population);
    static void AddExperimentData(int generation, const std::vector<SParticle*>& swarm);
    static void LogResultData(SSOIndividual& best, AProblem& problem);
    static SSOIndividual* FindBest(const std::vector<SSOIndividual *> &population);
    static SSOIndividual* FindBest(const std::vector<SParticle *> &swarm);
private:
    static std::string BestToCSVString(const SSOIndividual& best);
};
