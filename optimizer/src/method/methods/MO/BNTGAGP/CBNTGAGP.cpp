#include "CBNTGAGP.h"
#include "scheduler/Scheduler.hpp"
#include "problem/problems/MSRCPSP/CMSRCPSP_TA.h"
#include "problem/problems/MSRCPSP/CMSRCPSP_TO.h"
#include "utils/logger/CExperimentLogger.h"
#include <sstream>
#include "ImopseToBNTGAGP.h"
#include "gp/TreeEA.hpp"
#include "utils/random/CRandom.h"
#include "gp/FeatureScaling.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <vector>

CBNTGAGP::CBNTGAGP(AProblem* problem, AInitialization* initialization, SConfigMap* cfg)`r`n    : m_Cfg(cfg), m_Problem(problem), m_Initialization(initialization)
{
    if (m_Cfg && m_Cfg->HasValue("Seed")) {
        int s = 0;
        m_Cfg->TakeValue("Seed", s);
        m_HasSeedOverride = true;
        m_SeedOverride = (uint64_t)s;
    }
}

static int GetInt(SConfigMap* cfg, const char* key, int def)
{
    int v = def;
    if (cfg) cfg->TakeValue(key, v);
    return v;
}

static double GetDouble(SConfigMap* cfg, const char* key, double def)
{
    double v = def;
    if (cfg) cfg->TakeValue(key, v);
    return v;
}

void CBNTGAGP::RunOptimization()
{
    CScheduler* sch = nullptr;
    bool isTAProblem = false;

    if (auto* p = dynamic_cast<CMSRCPSP_TA*>(m_Problem)) { sch = &p->GetScheduler(); isTAProblem = true; }
    if (auto* p = dynamic_cast<CMSRCPSP_TO*>(m_Problem)) { sch = &p->GetScheduler(); isTAProblem = false; }

    if (!sch) {
        throw std::runtime_error("bNTGA-GP works only with MSRCPSP_TA/MSRCPSP_TO problems.");
    }

    SConfigMap cfgCopy;
    SConfigMap* cfg = nullptr;
    if (m_Cfg) { cfgCopy = *m_Cfg; cfg = &cfgCopy; }

    GPEA_Params P;
    P.popSize = 50;
    P.generations = 2000;
    P.pCrossover = 0.6;
    P.pMutParam = 0.3;
    P.pMutStruct = 0.02;
    P.maxDepth = 8;
    P.tournamentK = 2;
    P.useSinglePairTree = (GetInt(cfg, "UseSinglePairTree", 0) != 0);

    P.popSize = (size_t)GetInt(cfg, "PopulationSize", (int)P.popSize);
    P.generations = (size_t)GetInt(cfg, "Generations", (int)P.generations);
    P.pCrossover = GetDouble(cfg, "CrossoverProb", P.pCrossover);

    P.pMutParam = GetDouble(cfg, "MutationProbParam", P.pMutParam);
    P.pMutStruct = GetDouble(cfg, "MutationProbStruct", P.pMutStruct);
    P.pMutMacroSubtree = GetDouble(cfg, "MacroSubtreeProb", P.pMutMacroSubtree);

    {
        const double mutFallback = GetDouble(cfg, "MutationProb", P.pMutParam);
        P.pMutParam = GetDouble(cfg, "ParamMutationProb", mutFallback);
        P.pMutStruct = GetDouble(cfg, "StructMutationProb", P.pMutStruct);
    }

    P.maxDepth = GetInt(cfg, "MaxDepth", P.maxDepth);
    P.tournamentK = GetInt(cfg, "TournamentSize", P.tournamentK);

    P.logNodeDistribution = (GetInt(cfg, "LogNodeDistribution", 0) != 0);
    P.nodeStatsEvery = (size_t)GetInt(cfg, "NodeStatsEvery", 25);
    P.nodeStatsUseArchive = (GetInt(cfg, "NodeStatsUseArchive", 0) != 0);

    if (m_HasSeedOverride) {
        P.seed = m_SeedOverride;
    }
    else {
        P.seed = (uint64_t)CRandom::GetSeed();
    }

    Instance inst = BNTGAGPAdapter::FromScheduler(*sch);
    gp::initFeatureScaling(inst);

    TreeEA ea(inst, P, sch, isTAProblem);
    ea.run();

    const auto& gpArchive = ea.getArchive();

    std::vector<const GP_Individual*> gpArchiveSorted;
    gpArchiveSorted.reserve(gpArchive.size());
    for (const auto& ind : gpArchive) {
        gpArchiveSorted.push_back(&ind);
    }

    std::sort(gpArchiveSorted.begin(), gpArchiveSorted.end(),
        [](const GP_Individual* a, const GP_Individual* b) {
            if (a->makespan != b->makespan) return a->makespan < b->makespan;
            return a->cost < b->cost;
        });

    std::ostringstream oss;
    for (const GP_Individual* p : gpArchiveSorted) {
        oss << p->makespan << ';' << p->cost << '\n';
    }
    CExperimentLogger::LogResult(oss.str().c_str());
}