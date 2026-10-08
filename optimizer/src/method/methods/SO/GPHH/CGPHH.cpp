#include "CGPHH.h"
#include "scheduler/Scheduler.hpp"
#include "problem/problems/MSRCPSP/CMSRCPSP_TA.h"
#include "problem/problems/MSRCPSP/CMSRCPSP_TO.h"
#include "ImopseToGPHH.h"
#include "gp/FeatureScaling.hpp"
#include "gp/TreeEASO.hpp"
#include "utils/random/CRandom.h"
#include "utils/logger/CExperimentLogger.h"
#include "method/individual/SO/SSOIndividual.h"

#include <random>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>
#include <stdexcept>
#include <fstream>
#include <map>
#include <cmath>
#include <sstream>

namespace so_gp = gphh_so;

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

void CGPHH::RunOptimization()
{
    CScheduler* sch = nullptr;
    bool isTAProblem = false;

    if (auto* p = dynamic_cast<CMSRCPSP_TA*>(&m_Problem)) { sch = &p->GetScheduler(); isTAProblem = true; }
    if (auto* p = dynamic_cast<CMSRCPSP_TO*>(&m_Problem)) { sch = &p->GetScheduler(); isTAProblem = false; }

    if (!sch) {
        throw std::runtime_error("GP-HH works only with MSRCPSP_TA/MSRCPSP_TO problems.");
    }

    SConfigMap cfgCopy;
    SConfigMap* cfg = nullptr;
    if (m_Cfg) { cfgCopy = *m_Cfg; cfg = &cfgCopy; }

    so_gp::GPEA_Params P;
    P.popSize = 50;
    P.generations = 2000;
    P.pCrossover = 0.6;
    P.pMutation = 0.3;

    P.pSubtreeMutation = 0.34;
    P.pPointMutation = 0.33;
    P.pHoistMutation = 0.33;
    P.pFeatureGuidedPointMutation = 0.0;
    P.pMacroSubtreeMutation = 0.0;

    P.pSubtreeCrossover = 1.0;
    P.pFeatureAwareCrossover = 0.0;

    P.maxDepth = 8;
    P.tournamentK = 2;
    P.eliteCount = 1;
    P.useGeneLevelMutation = false;
    P.logPopulationDiversity = false;
    P.avoidClonesInPopulation = false;
    P.cloneMutationRetries = 3;

    P.useImopseEvaluate = (GetInt_SO(cfg, "UseImopseEvaluate", 1) != 0);
    P.useSinglePairTree = (GetInt_SO(cfg, "UseSinglePairTree", 0) != 0);
    P.useGeneLevelMutation = (GetInt_SO(cfg, "UseGeneLevelMutation", (int)P.useGeneLevelMutation) != 0);
    P.logPopulationDiversity = (GetInt_SO(cfg, "LogPopulationDiversity", (int)P.logPopulationDiversity) != 0);
    P.avoidClonesInPopulation = (GetInt_SO(cfg, "AvoidClonesInPopulation", (int)P.avoidClonesInPopulation) != 0);
    P.cloneMutationRetries = GetInt_SO(cfg, "CloneMutationRetries", P.cloneMutationRetries);

    P.popSize = (size_t)GetInt_SO(cfg, "PopulationSize", (int)P.popSize);
    P.generations = (size_t)GetInt_SO(cfg, "Generations", (int)P.generations);
    P.pCrossover = GetDouble_SO(cfg, "CrossoverProb", P.pCrossover);
    P.pMutation = GetDouble_SO(cfg, "MutationProb", P.pMutation);

    P.pSubtreeMutation = GetDouble_SO(cfg, "SubtreeMutationShare", P.pSubtreeMutation);
    P.pPointMutation = GetDouble_SO(cfg, "PointMutationShare", P.pPointMutation);
    P.pHoistMutation = GetDouble_SO(cfg, "HoistMutationShare", P.pHoistMutation);
    P.pFeatureGuidedPointMutation = GetDouble_SO(cfg, "FeatureGuidedPointMutationShare", P.pFeatureGuidedPointMutation);
    P.pMacroSubtreeMutation = GetDouble_SO(cfg, "MacroSubtreeMutationProb", P.pMacroSubtreeMutation);

    P.pSubtreeCrossover = GetDouble_SO(cfg, "SubtreeCrossoverShare", P.pSubtreeCrossover);
    P.pFeatureAwareCrossover = GetDouble_SO(cfg, "FeatureAwareCrossoverShare", P.pFeatureAwareCrossover);

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

    so_gp::Instance inst = so_gp::GPHHAdapter::FromScheduler(*sch);
    so_gp::gp::initFeatureScaling(inst);

    so_gp::TreeEASO ea(inst, P, sch, isTAProblem);
    so_gp::GP_Individual best = ea.run();

    {
        so_gp::Instance profileInst = so_gp::GPHHAdapter::FromScheduler(*sch);

        so_gp::GPTreeRule gpTaskRule(best.taskTree);
        so_gp::GPTreeResRule gpResRule(best.resTree);

        const so_gp::IDispatchingRule& taskRule =
            static_cast<const so_gp::IDispatchingRule&>(gpTaskRule);

        const so_gp::GPTreeResRule* resRule =
            P.useSinglePairTree ? nullptr : &gpResRule;

        const so_gp::GPTree* pairTree =
            P.useSinglePairTree ? &best.taskTree : nullptr;

        so_gp::ScheduleOptions profileOpt;
        profileOpt.computeObjectiveStats = false;
        profileOpt.keepTaskAssignedResources = false;
        profileOpt.captureAssignedResByImopse = (P.useImopseEvaluate && isTAProblem);
        profileOpt.featureProfile = true;
        profileOpt.featureProfileCsvPath =
            CExperimentLogger::m_OutputDataPathPrefix + "/feature_profile_best.csv";

        so_gp::ScheduleResult profileSim = so_gp::Scheduler::withResources(
            profileInst,
            taskRule,
            resRule,
            profileOpt,
            pairTree
        );

        if (P.useImopseEvaluate && isTAProblem) {
            sch->Reset();

            const size_t n = sch->GetTasks().size();
            for (size_t i = 0; i < n; ++i) {
                const TResourceID resId =
                    (TResourceID)profileSim.assignedResByImopseTaskIndex.at(i);
                sch->Assign(i, resId);
            }

            sch->BuildTimestamps_TA();

            SGenotype dummyGenotype;
            std::vector<float> eval = {
                (float)sch->EvaluateDuration(),
                (float)sch->EvaluateCost()
            };
            std::vector<float> norm = { 0.0f, 0.0f };

            SSOIndividual outSol(dummyGenotype, eval, norm);
            outSol.m_Fitness = (float)best.fitness;

            CExperimentLogger::WriteSchedulerToFile(*sch, outSol);
        }
    }

    CExperimentLogger::LogData();

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