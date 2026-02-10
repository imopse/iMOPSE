#include "CGPHH.h"
#include "utils/logger/CExperimentLogger.h"
#include "problem/problems/MSRCPSP/CMSRCPSP_TA.h"
#include "problem/problems/MSRCPSP/CMSRCPSP_TO.h"
#include "ImopseToGPHH.h"
#include "gp/TreeEA.hpp"
#include <string>
#include <stdexcept>

extern bool g_trace;

CGPHH::CGPHH(AProblem& problem, AInitialization& init, SConfigMap* cfg)
    : AMethod(problem, init), m_Cfg(cfg) {
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

void CGPHH::RunOptimization()
{
    const CScheduler* sch = nullptr;
    if (auto* p = dynamic_cast<CMSRCPSP_TA*>(&m_Problem)) sch = &p->GetScheduler();
    if (auto* p = dynamic_cast<CMSRCPSP_TO*>(&m_Problem)) sch = &p->GetScheduler();

    if (!sch) {
        throw std::runtime_error("GPHH works only with MSRCPSP_TA/MSRCPSP_TO problems.");
    }

    GPEA_Params P;

    P.popSize = (size_t)GetInt(m_Cfg, "PopulationSize", (int)P.popSize);
    P.generations = (size_t)GetInt(m_Cfg, "Generations", (int)P.generations);

    P.pCrossover = GetDouble(m_Cfg, "CrossoverProb", P.pCrossover);

    const double mutDefault = GetDouble(m_Cfg, "MutationProb", P.pMutParam);
    P.pMutParam = GetDouble(m_Cfg, "ParamMutationProb", mutDefault);
    P.pMutStruct = GetDouble(m_Cfg, "StructMutationProb", P.pMutStruct); 

    P.maxDepth = GetInt(m_Cfg, "MaxDepth", P.maxDepth);
    P.weight = GetDouble(m_Cfg, "Weight", P.weight);

    P.tournamentK = GetInt(m_Cfg, "TournamentSize", P.tournamentK);
    P.eliteCount = (size_t)GetInt(m_Cfg, "EliteCount", (int)P.eliteCount);

    P.useNormalization = (GetInt(m_Cfg, "UseNormalization", (int)P.useNormalization) != 0);

    g_trace = (GetInt(m_Cfg, "Trace", 0) != 0);

    P.seed = (uint64_t)GetInt(m_Cfg, "Seed", 123);


    Instance inst = GPHHAdapter::FromScheduler(*sch);

    TreeEA ea(inst, P);
    auto best = ea.run();

    std::string msg =
        "Best: makespan=" + std::to_string(best.makespan) +
        " cost=" + std::to_string(best.cost) +
        " msNorm=" + std::to_string(best.msNorm) +
        " costNorm=" + std::to_string(best.costNorm) + "\n";

    CExperimentLogger::LogResult(msg.c_str(), "gphh_summary.txt");
}
