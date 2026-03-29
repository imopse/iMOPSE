#include "CBNTGAGPSO.h"
#include "scheduler/Scheduler.hpp"
#include "problem/problems/MSRCPSP/CMSRCPSP_TA.h"
#include "problem/problems/MSRCPSP/CMSRCPSP_TO.h"
#include "ImopseToBNTGAGP.h"
#include "gp/FeatureScaling.hpp"
#include "gp/TreeEASO.hpp"
#include "utils/random/CRandom.h"
#include "utils/logger/CExperimentLogger.h"

#include <random>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>
#include <stdexcept>
#include <sstream>

namespace so_gp = gpbntga_so;

CBNTGAGPSO::CBNTGAGPSO(AProblem& problem, AInitialization& init, SConfigMap* cfg)
    : AMethod(problem, init), m_Cfg(cfg)
{
    if (m_Cfg && m_Cfg->HasValue("Seed")) {
        int s = 0;
        m_Cfg->TakeValue("Seed", s);
        m_HasSeedOverride = true;
        m_SeedOverride = (uint64_t)s;
    }
}

static int GetInt_SO(SConfigMap* cfg, const char* key, int def)
{
    int v = def;
    if (cfg) cfg->TakeValue(key, v);
    return v;
}

static double GetDouble_SO(SConfigMap* cfg, const char* key, double def)
{
    double v = def;
    if (cfg) cfg->TakeValue(key, v);
    return v;
}

void CBNTGAGPSO::RunOptimization()
{
    CScheduler* sch = nullptr;
    bool isTAProblem = false;

    if (auto* p = dynamic_cast<CMSRCPSP_TA*>(&m_Problem)) { sch = &p->GetScheduler(); isTAProblem = true; }
    if (auto* p = dynamic_cast<CMSRCPSP_TO*>(&m_Problem)) { sch = &p->GetScheduler(); isTAProblem = false; }

    if (!sch) {
        throw std::runtime_error("bNTGA-GP-SO works only with MSRCPSP_TA/MSRCPSP_TO problems.");
    }

    SConfigMap cfgCopy;
    SConfigMap* cfg = nullptr;
    if (m_Cfg) { cfgCopy = *m_Cfg; cfg = &cfgCopy; }

    so_gp::GPEA_Params P;
    P.popSize = 50;
    P.generations = 2000;
    P.pCrossover = 0.6;
    P.pMutParam = 0.3;
    P.pMutStruct = 0.02;
    P.maxDepth = 8;
    P.tournamentK = 2;
    P.eliteCount = 1;

    P.useImopseEvaluate = (GetInt_SO(cfg, "UseImopseEvaluate", 1) != 0);
    P.useSinglePairTree = (GetInt_SO(cfg, "UseSinglePairTree", 0) != 0);

    P.popSize = (size_t)GetInt_SO(cfg, "PopulationSize", (int)P.popSize);
    P.generations = (size_t)GetInt_SO(cfg, "Generations", (int)P.generations);
    P.pCrossover = GetDouble_SO(cfg, "CrossoverProb", P.pCrossover);

    P.pMutParam = GetDouble_SO(cfg, "MutationProbParam", P.pMutParam);
    P.pMutStruct = GetDouble_SO(cfg, "MutationProbStruct", P.pMutStruct);
    P.pMutMacroSubtree = GetDouble_SO(cfg, "MacroSubtreeProb", P.pMutMacroSubtree);

    {
        const double mutFallback = GetDouble_SO(cfg, "MutationProb", P.pMutParam);
        P.pMutParam = GetDouble_SO(cfg, "ParamMutationProb", mutFallback);
        P.pMutStruct = GetDouble_SO(cfg, "StructMutationProb", P.pMutStruct);
    }

    P.maxDepth = GetInt_SO(cfg, "MaxDepth", P.maxDepth);
    P.weight = GetDouble_SO(cfg, "Weight", P.weight);
    P.tournamentK = GetInt_SO(cfg, "TournamentSize", P.tournamentK);
    P.eliteCount = (size_t)GetInt_SO(cfg, "EliteCount", (int)P.eliteCount);
    P.useNormalization = (GetInt_SO(cfg, "UseNormalization", (int)P.useNormalization) != 0);

    if (m_HasSeedOverride) {
        P.seed = m_SeedOverride;
    }
    else {
        P.seed = (uint64_t)CRandom::GetSeed();
    }

    so_gp::Instance inst = so_gp::BNTGAGPAdapter::FromScheduler(*sch);
    so_gp::gp::initFeatureScaling(inst);

    so_gp::TreeEASO ea(inst, P, sch, isTAProblem);
    so_gp::GP_Individual best = ea.run();

    {
        std::ostringstream oss;
        oss << best.fitness << ';'
            << best.makespan << ';'
            << best.cost << ';'
            << best.msNorm << ';'
            << best.costNorm;
        CExperimentLogger::LogResult(oss.str().c_str());
    }
}