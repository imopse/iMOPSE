#include "CGPHH.h"
#include "scheduler/Scheduler.hpp"
#include "utils/logger/CExperimentLogger.h"
#include "problem/problems/MSRCPSP/CMSRCPSP_TA.h"
#include "problem/problems/MSRCPSP/CMSRCPSP_TO.h"
#include "../utils/archive/ArchiveUtils.h"
#include "ImopseToGPHH.h"
#include "gp/TreeEA.hpp"
#include "utils/random/CRandom.h"
#include "gp/FeatureScaling.hpp"
#include <random>
#include <algorithm>
#include <cctype>
#include <limits>
#include <cstdint>
#include <string>
#include <stdexcept>
#include <sstream>
#include <vector>

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
    CScheduler* sch = nullptr;
    bool isTAProblem = false;

    if (auto* p = dynamic_cast<CMSRCPSP_TA*>(&m_Problem)) { sch = &p->GetScheduler(); isTAProblem = true; }
    if (auto* p = dynamic_cast<CMSRCPSP_TO*>(&m_Problem)) { sch = &p->GetScheduler(); isTAProblem = false; }

    if (!sch) {
        throw std::runtime_error("GPHH works only with MSRCPSP_TA/MSRCPSP_TO problems.");
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
    P.eliteCount = 0;
    P.useNSGA2 = (GetInt(cfg, "UseNSGA2", 0) != 0);
    P.useBNTGA = (GetInt(cfg, "UseBNTGA", 0) != 0);
    P.useImopseEvaluate = (GetInt(cfg, "UseImopseEvaluate", 1) != 0);
    const bool enableFinalDiagnostics = (GetInt(cfg, "EnableFinalDiagnostics", 0) != 0);


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
    P.weight = GetDouble(cfg, "Weight", P.weight);

    P.tournamentK = GetInt(cfg, "TournamentSize", P.tournamentK);
    P.eliteCount = (size_t)GetInt(cfg, "EliteCount", (int)P.eliteCount);

    P.useNormalization = (GetInt(cfg, "UseNormalization", (int)P.useNormalization) != 0);

    g_trace = (GetInt(cfg, "Trace", 0) != 0);

    if (m_HasSeedOverride) {
        P.seed = m_SeedOverride;
    }
    else {
        P.seed = (uint64_t)CRandom::GetSeed();
    }

    Instance inst = GPHHAdapter::FromScheduler(*sch);
    gp::initFeatureScaling(inst);

    const bool useBaseline = (GetInt(cfg, "UseBaseline", 1) != 0);
    const int  seedDepth = GetInt(cfg, "SeedDepth", 3);
    std::string startRule = ToLower(GetString(cfg, "StartRule", "random"));

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

    TreeEA ea(inst, P, sch, isTAProblem);
    if (useBaseline) ea.setSeedTrees(startTreeTask, startTreeRes);

    auto best = ea.run();

    {
        if (enableFinalDiagnostics) {
            ResChoiceDiag diag;
            Scheduler::setResChoiceDiag(&diag);

            Instance Idbg = inst;
            GPTreeRule    dbgT(best.taskTree);
            GPTreeResRule dbgR(best.resTree);
            (void)Scheduler::withResources(Idbg, dbgT, &dbgR);

            Scheduler::setResChoiceDiag(nullptr);

            std::ostringstream d;
            d << "calls=" << diag.calls << "\n";
            d << "emptyCalls=" << diag.emptyCalls << "\n";
            if (diag.calls > 0) {
                d << "avgCandidates=" << (double)diag.sumCandidates / (double)diag.calls << "\n";
                d << "avgUniqueScores=" << (double)diag.sumUniqueScores / (double)diag.calls << "\n";
                d << "avgTiesMin=" << (double)diag.sumTiesMin / (double)diag.calls << "\n";
                if (diag.sumCandidates > 0) {
                    d << "minTieRate=" << (double)diag.sumTiesMin / (double)diag.sumCandidates << "\n";
                }
            }

            CExperimentLogger::LogResult(d.str().c_str(), "res_rule_diag.txt");

            {
                ResFeatureDiag rfd;
                TaskFeatureDiag tfd;

                Scheduler::setResFeatureDiag(&rfd);
                Scheduler::setTaskFeatureDiag(&tfd);

                Instance Idbg = inst;
                GPTreeRule    dbgT(best.taskTree);
                GPTreeResRule dbgR(best.resTree);
                (void)Scheduler::withResources(Idbg, dbgT, &dbgR);

                Scheduler::setResFeatureDiag(nullptr);
                Scheduler::setTaskFeatureDiag(nullptr);

                auto dumpTable = [](std::ostringstream& out,
                    const auto& names,
                    const auto& stats) {
                        out << "name;groups;avgCandidates;avgUnique;pctAllEqual;pctAllZero;avgRange;pctGroupsNonFinite;pctValuesNonFinite\n";
                        for (size_t i = 0; i < names.size(); ++i) {
                            const auto& st = stats[i];
                            double g = (double)st.groups;
                            double avgCand = (g > 0) ? (double)st.sumCandidates / g : 0.0;
                            double avgUniq = (g > 0) ? (double)st.sumUnique / g : 0.0;
                            double pctEq = (g > 0) ? 100.0 * (double)st.groupsAllEqual / g : 0.0;
                            double pct0 = (g > 0) ? 100.0 * (double)st.groupsAllZero / g : 0.0;
                            double avgR = (g > 0) ? st.sumRange / g : 0.0;
                            double pctGNF = (g > 0) ? 100.0 * (double)st.groupsWithNonFinite / g : 0.0;
                            double pctVNF = (st.sumCandidates > 0) ? 100.0 * (double)st.nonFiniteValues / (double)st.sumCandidates : 0.0;

                            out << names[i] << ";"
                                << st.groups << ";"
                                << avgCand << ";"
                                << avgUniq << ";"
                                << pctEq << ";"
                                << pct0 << ";"
                                << avgR << ";"
                                << pctGNF << ";"
                                << pctVNF << "\n";
                        }
                    };

                {
                    std::ostringstream out;
                    dumpTable(out, RES_FEAT_NAMES, rfd.st);
                    CExperimentLogger::LogResult(out.str().c_str(), "feature_diag_res.txt");
                }
                {
                    std::ostringstream out;
                    dumpTable(out, TASK_FEAT_NAMES, tfd.st);
                    CExperimentLogger::LogResult(out.str().c_str(), "feature_diag_task.txt");
                }
            }
        }
    }

    const auto& pf = ea.getPareto();

    std::vector<GP_ParetoPoint> pfSorted = pf;
    std::sort(pfSorted.begin(), pfSorted.end(),
        [](const GP_ParetoPoint& a, const GP_ParetoPoint& b) {
            if (a.makespan != b.makespan) return a.makespan < b.makespan;
            return a.cost < b.cost;
        });

    std::vector<SMOIndividual*> archive;
    archive.reserve(pfSorted.size());

    for (const auto& p : pfSorted) {
        SGenotype g;

        std::vector<float> eval = {
            (float)p.makespan,
            (float)p.cost
        };

        std::vector<float> norm = {
            (float)p.msNorm,
            (float)p.costNorm
        };

        archive.push_back(new SMOIndividual(g, eval, norm));
    }

    ArchiveUtils::LogParetoFront(archive);

    for (auto* ind : archive) delete ind;
    archive.clear();



    {
        std::vector<GP_ParetoPoint> pf = ea.getPareto();
        std::sort(pf.begin(), pf.end(), [](const GP_ParetoPoint& a, const GP_ParetoPoint& b) {
            if (a.msNorm != b.msNorm) return a.msNorm < b.msNorm;
            return a.costNorm < b.costNorm;
            });

        std::ostringstream oss;
        for (const auto& p : pf) {
            oss << p.msNorm << ";" << p.costNorm << "\n";
        }
        CExperimentLogger::LogResult(oss.str().c_str(), "results_norm.csv");
    }

}
