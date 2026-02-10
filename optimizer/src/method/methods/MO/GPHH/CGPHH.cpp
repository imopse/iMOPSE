#include "CGPHH.h"
#include "utils/logger/CExperimentLogger.h"
#include "problem/problems/MSRCPSP/CMSRCPSP_TA.h"
#include "problem/problems/MSRCPSP/CMSRCPSP_TO.h"
#include "ImopseToGPHH.h"
#include "gp/TreeEA.hpp"
#include "utils/random/CRandom.h"
#include "gp/Features.hpp"
#include <random>
#include <algorithm>
#include <cctype>
#include <limits>
#include <cstdint>
#include <string>
#include <stdexcept>

extern bool g_trace;

CGPHH::CGPHH(AProblem& problem, AInitialization& init, SConfigMap* cfg)
    : AMethod(problem, init), m_Cfg(cfg)
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

static std::string GetString(SConfigMap* cfg, const char* key, const std::string& def)
{
    std::string v = def;
    if (cfg) cfg->TakeValue(key, v);
    return v;
}

static std::string ToLower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return (char)std::tolower(c); });
    return s;
}

void CGPHH::RunOptimization()
{
    const CScheduler* sch = nullptr;
    if (auto* p = dynamic_cast<CMSRCPSP_TA*>(&m_Problem)) sch = &p->GetScheduler();
    if (auto* p = dynamic_cast<CMSRCPSP_TO*>(&m_Problem)) sch = &p->GetScheduler();

    if (!sch) {
        throw std::runtime_error("GPHH works only with MSRCPSP_TA/MSRCPSP_TO problems.");
    }

    GPEA_Params P;
    P.popSize = 50;
    P.generations = 2000;
    P.pCrossover = 0.6;
    P.pMutParam = 0.3;
    P.pMutStruct = 0.02;
    P.maxDepth = 8;
    P.tournamentK = 2;
    P.eliteCount = 0;


    P.popSize = (size_t)GetInt(m_Cfg, "PopulationSize", (int)P.popSize);
    P.generations = (size_t)GetInt(m_Cfg, "Generations", (int)P.generations);
    P.pCrossover = GetDouble(m_Cfg, "CrossoverProb", P.pCrossover);

    P.pMutParam = GetDouble(m_Cfg, "MutationProbParam", P.pMutParam);
    P.pMutStruct = GetDouble(m_Cfg, "MutationProbStruct", P.pMutStruct);

    {
        const double mutFallback = GetDouble(m_Cfg, "MutationProb", P.pMutParam);
        P.pMutParam = GetDouble(m_Cfg, "ParamMutationProb", mutFallback);
        P.pMutStruct = GetDouble(m_Cfg, "StructMutationProb", P.pMutStruct);
    }

    P.maxDepth = GetInt(m_Cfg, "MaxDepth", P.maxDepth);
    P.weight = GetDouble(m_Cfg, "Weight", P.weight);

    P.tournamentK = GetInt(m_Cfg, "TournamentSize", P.tournamentK);
    P.eliteCount = (size_t)GetInt(m_Cfg, "EliteCount", (int)P.eliteCount);

    P.useNormalization = (GetInt(m_Cfg, "UseNormalization", (int)P.useNormalization) != 0);

    g_trace = (GetInt(m_Cfg, "Trace", 0) != 0);

    if (m_HasSeedOverride) {
        P.seed = m_SeedOverride;
    }
    else {
        P.seed = (uint64_t)CRandom::GetInt(1, std::numeric_limits<int>::max());
    }

    Instance inst = GPHHAdapter::FromScheduler(*sch);
    gp::initFeatureScaling(inst);

    const bool useBaseline = (GetInt(m_Cfg, "UseBaseline", 1) != 0);
    const int  seedDepth = GetInt(m_Cfg, "SeedDepth", 3);
    std::string startRule = ToLower(GetString(m_Cfg, "StartRule", "random"));

    std::mt19937 rng((unsigned)P.seed);

    GPTree startTreeTask;
    if (startRule == "random")                  startTreeTask = GPTree::RandomTreeMS(rng, seedDepth);
    else if (startRule == "avail-gap")          startTreeTask = GPTree::Make_AVAIL_minus_REQ();
    else if (startRule == "work")               startTreeTask = GPTree::Make_REQ_times_DUR();
    else if (startRule == "est+dur")            startTreeTask = GPTree::Make_EST_plus_DUR();
    else if (startRule == "cheapxdur")          startTreeTask = GPTree::Make_CHEAPxDUR();
    else if (startRule == "cheap-per-skill+est")startTreeTask = GPTree::Make_CHEAP_PER_SKILL_plus_EST();
    else                                        startTreeTask = GPTree::Make_AVAIL_minus_REQ();

    GPTree startTreeRes = GPTree::RandomTreeRES(rng, seedDepth);

    TreeEA ea(inst, P);
    if (useBaseline) ea.setSeedTrees(startTreeTask, startTreeRes);

    auto best = ea.run();

    if (useBaseline) {
        std::string s =
            "Baseline=ON StartRule=" + startRule +
            " SeedDepth=" + std::to_string(seedDepth) + "\n";
        CExperimentLogger::LogResult(s.c_str(), "gphh_params.txt");
    }
    else {
        CExperimentLogger::LogResult("Baseline=OFF\n", "gphh_params.txt");
    }

    std::string msg =
        "SeedUsed=" + std::to_string((unsigned long long)P.seed) +
        " Best: makespan=" + std::to_string(best.makespan) +
        " cost=" + std::to_string(best.cost) + "\n";
    CExperimentLogger::LogResult(msg.c_str(), "gphh_summary.txt");
}
