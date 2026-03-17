#include "TreeEA.hpp"
#include "../rules/GPTreeRule.hpp"
#include "../rules/GPTreeResRule.hpp"
#include "problem/problems/MSRCPSP/CScheduler.h"
#include <cmath>
#include <cassert>
#include <algorithm>
#include <numeric>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <map>
#include <iostream>
#include "../../../../../utils/logger/CExperimentLogger.h"

extern bool g_trace;

static bool dominates(const GP_ParetoPoint& a, const GP_ParetoPoint& b) {
    const bool noWorse = (a.makespan <= b.makespan) && (a.cost <= b.cost);
    const bool strictlyBetter = (a.makespan < b.makespan) || (a.cost < b.cost);
    return noWorse && strictlyBetter;
}

static bool samePoint(const GP_ParetoPoint& a, const GP_ParetoPoint& b) {
    return (a.makespan == b.makespan) && (std::abs(a.cost - b.cost) < 1e-9);
}

static const std::vector<FeatureId>& taskFeatPool() {
    static const std::vector<FeatureId> v = GPTree::allFeatures();
    return v;
}

static const std::vector<FeatureId>& resFeatPool() {
    static const std::vector<FeatureId> v = {
        FeatureId::RES_WAGE,
        FeatureId::RES_SKILL_LEVEL,
        FeatureId::RES_WAIT_TIME,
        FeatureId::RES_CAN_START_NOW,
        FeatureId::RES_WAGE_PER_LEVEL,
        FeatureId::RES_ASSIGN_COST,
        FeatureId::RES_ASSIGN_PREMIUM_ALL,
        FeatureId::RES_HASTE_VALUE,
        FeatureId::RES_RESERVE_PRESSURE,
        FeatureId::RES_STRATEGIC_MISMATCH,
        FeatureId::RES_FAMILY_MISMATCH,
        FeatureId::RES_SURPLUS_LEVEL,
        FeatureId::RES_RELATIVE_WAGE
    };
    return v;
}

struct GenLogOpCounts {
    int crossoverChildren = 0;
    int paramMutChildren = 0;
    int structMutChildren = 0;
    int macroMutChildren = 0;
};

struct ChildOpFlags {
    bool crossover = false;
    bool paramMut = false;
    bool structMut = false;
    bool macroMut = false;
};

struct GenLogArchiveEntryCounts {
    int totalEntries = 0;
    int entriesByCrossover = 0;
    int entriesByParamMut = 0;
    int entriesByStructMut = 0;
    int entriesByMacroMut = 0;
    int entriesLe700 = 0;
    int entriesLe800 = 0;
};

struct LoggedChild {
    GP_Individual ind;
    ChildOpFlags ops;
};

static std::string makeGenerationLogPath(const GPEA_Params& P) {
    namespace fs = std::filesystem;

    fs::path dir = CExperimentLogger::m_OutputDataPathPrefix.empty()
        ? fs::current_path()
        : fs::path(CExperimentLogger::m_OutputDataPathPrefix);

    return (dir / P.generationLogFile).string();
}

static void writeGenerationLogHeader(const GPEA_Params& P) {
    if (!P.logGenerations) return;

    std::ofstream out(makeGenerationLogPath(P), std::ofstream::out | std::ofstream::trunc);
    if (!out.is_open()) return;

    out << "generation"
        << ",pop_size"
        << ",archive_size"
        << ",archive_delta"
        << ",best_fitness"
        << ",best_makespan"
        << ",best_cost"
        << ",best_cost_le_600"
        << ",best_cost_le_700"
        << ",best_cost_le_800"
        << ",best_cost_le_1000"
        << ",best_cost_le_1300"
        << ",avg_task_depth"
        << ",avg_res_depth"
        << ",avg_task_nodes"
        << ",avg_res_nodes"
        << ",children_with_crossover"
        << ",children_with_param_mut"
        << ",children_with_struct_mut"
        << ",children_with_macro_mut"
        << ",archive_entries_total"
        << ",archive_entries_by_crossover"
        << ",archive_entries_by_param_mut"
        << ",archive_entries_by_struct_mut"
        << ",archive_entries_by_macro_mut"
        << ",archive_entries_le_700"
        << ",archive_entries_le_800"
        << "\n";
}

static double bestCostUnderMakespan(const std::vector<GP_Individual>& pop, int maxMakespan) {
    double best = std::numeric_limits<double>::infinity();

    for (const auto& x : pop) {
        if (x.makespan <= maxMakespan && x.cost < best) {
            best = x.cost;
        }
    }

    return std::isfinite(best) ? best : -1.0;
}

static int countUnderMakespan(const std::vector<GP_Individual>& xs, int maxMakespan) {
    int cnt = 0;
    for (const auto& x : xs) {
        if (x.makespan <= maxMakespan) ++cnt;
    }
    return cnt;
}

static int bestMakespanInSet(const std::vector<GP_Individual>& xs) {
    if (xs.empty()) return -1;

    int best = std::numeric_limits<int>::max();
    for (const auto& x : xs) {
        if (x.makespan < best) best = x.makespan;
    }
    return (best == std::numeric_limits<int>::max()) ? -1 : best;
}

static double bestCostInSet(const std::vector<GP_Individual>& xs) {
    if (xs.empty()) return -1.0;

    double best = std::numeric_limits<double>::infinity();
    for (const auto& x : xs) {
        if (x.cost < best) best = x.cost;
    }
    return std::isfinite(best) ? best : -1.0;
}

static void appendGenerationLog(
    const GPEA_Params& P,
    size_t generation,
    const std::vector<GP_Individual>& pop,
    size_t archiveSize,
    long long archiveDelta,
    const GenLogOpCounts& ops,
    const GenLogArchiveEntryCounts& archiveOps)
{
    if (!P.logGenerations || pop.empty()) return;

    auto itBest = std::min_element(pop.begin(), pop.end(),
        [](const GP_Individual& a, const GP_Individual& b) {
            return a.fitness < b.fitness;
        });

    double sumTaskDepth = 0.0;
    double sumResDepth = 0.0;
    double sumTaskNodes = 0.0;
    double sumResNodes = 0.0;

    for (const auto& ind : pop) {
        sumTaskDepth += ind.taskTree.depth();
        sumResDepth += ind.resTree.depth();
        sumTaskNodes += ind.taskTree.nodeCount();
        sumResNodes += ind.resTree.nodeCount();
    }

    const double denom = std::max<size_t>(1, pop.size());

    const double avgTaskDepth = sumTaskDepth / denom;
    const double avgResDepth = sumResDepth / denom;
    const double avgTaskNodes = sumTaskNodes / denom;
    const double avgResNodes = sumResNodes / denom;

    std::ofstream out(makeGenerationLogPath(P), std::ofstream::out | std::ofstream::app);
    if (!out.is_open()) return;

    out << std::fixed << std::setprecision(6);

    out << generation
        << "," << pop.size()
        << "," << archiveSize
        << "," << archiveDelta
        << "," << itBest->fitness
        << "," << itBest->makespan
        << "," << itBest->cost
        << "," << bestCostUnderMakespan(pop, 600)
        << "," << bestCostUnderMakespan(pop, 700)
        << "," << bestCostUnderMakespan(pop, 800)
        << "," << bestCostUnderMakespan(pop, 1000)
        << "," << bestCostUnderMakespan(pop, 1300)
        << "," << avgTaskDepth
        << "," << avgResDepth
        << "," << avgTaskNodes
        << "," << avgResNodes
        << "," << ops.crossoverChildren
        << "," << ops.paramMutChildren
        << "," << ops.structMutChildren
        << "," << ops.macroMutChildren
        << "," << archiveOps.totalEntries
        << "," << archiveOps.entriesByCrossover
        << "," << archiveOps.entriesByParamMut
        << "," << archiveOps.entriesByStructMut
        << "," << archiveOps.entriesByMacroMut
        << "," << archiveOps.entriesLe700
        << "," << archiveOps.entriesLe800
        << "\n";
}

static std::string makeArchiveDumpPath(const GPEA_Params& P) {
    namespace fs = std::filesystem;
    fs::path genPath = makeGenerationLogPath(P);
    return (genPath.parent_path() / "gphh_archive_dump.txt").string();
}

static std::string makeSchedulerTracePath(const GPEA_Params& P, const std::string& fileName) {
    namespace fs = std::filesystem;
    fs::path genPath = makeGenerationLogPath(P);
    return (genPath.parent_path() / fileName).string();
}

static const char* featureIdToStringLocal(FeatureId f) {
    switch (f) {
    case FeatureId::DURATION: return "DURATION";
    case FeatureId::REQ_LEVEL: return "REQ_LEVEL";
    case FeatureId::AVAIL_SKILL: return "AVAIL_SKILL";
    case FeatureId::EST_PREC: return "EST_PREC";
    case FeatureId::SUCC_COUNT: return "SUCC_COUNT";
    case FeatureId::DESC_COUNT: return "DESC_COUNT";
    case FeatureId::CRITLEN: return "CRITLEN";
    case FeatureId::SLACK: return "SLACK";
    case FeatureId::CRITICAL_PRESSURE: return "CRITICAL_PRESSURE";
    case FeatureId::AVAIL_GAP: return "AVAIL_GAP";
    case FeatureId::WAIT_RES: return "WAIT_RES";
    case FeatureId::TOT_PRED: return "TOT_PRED";
    case FeatureId::CHEAPEST_COST_NOW: return "CHEAPEST_COST_NOW";
    case FeatureId::COST_PER_SKILL_NOW: return "COST_PER_SKILL_NOW";
    case FeatureId::MIN_WAGE_AVAIL: return "MIN_WAGE_AVAIL";
    case FeatureId::AVG_WAGE_AVAIL: return "AVG_WAGE_AVAIL";
    case FeatureId::TEAM_SIZE_MIN_NOW: return "TEAM_SIZE_MIN_NOW";
    case FeatureId::NUM_TASKS: return "NUM_TASKS";
    case FeatureId::NUM_RESOURCES: return "NUM_RESOURCES";
    case FeatureId::NUM_SKILLS: return "NUM_SKILLS";
    case FeatureId::TASK_RES_COUNT: return "TASK_RES_COUNT";
    case FeatureId::AVG_RES_COST: return "AVG_RES_COST";
    case FeatureId::UNSCHED_TASKS: return "UNSCHED_TASKS";
    case FeatureId::MIN_FEASIBLE_COST_NOW: return "MIN_FEASIBLE_COST_NOW";
    case FeatureId::COST_REGRET_NOW: return "COST_REGRET_NOW";

    case FeatureId::RES_WAGE: return "RES_WAGE";
    case FeatureId::RES_SKILL_LEVEL: return "RES_SKILL_LEVEL";
    case FeatureId::RES_WAIT_TIME: return "RES_WAIT_TIME";
    case FeatureId::RES_IDLE_TIME: return "RES_IDLE_TIME";
    case FeatureId::RES_CAN_START_NOW: return "RES_CAN_START_NOW";
    case FeatureId::RES_UTILIZATION: return "RES_UTILIZATION";
    case FeatureId::RES_WAGE_PER_LEVEL: return "RES_WAGE_PER_LEVEL";
    case FeatureId::RES_ASSIGN_COST: return "RES_ASSIGN_COST";
    case FeatureId::RES_ASSIGN_PREMIUM_ALL: return "RES_ASSIGN_PREMIUM_ALL";
    case FeatureId::RES_HASTE_VALUE: return "RES_HASTE_VALUE";
    case FeatureId::RES_CRITICAL_RESERVE: return "RES_CRITICAL_RESERVE";
    case FeatureId::RES_RESERVE_PRESSURE: return "RES_RESERVE_PRESSURE";
    case FeatureId::RES_STRATEGIC_MISMATCH: return "RES_STRATEGIC_MISMATCH";
    case FeatureId::RES_FAMILY_MISMATCH: return "RES_FAMILY_MISMATCH";
    case FeatureId::RES_SURPLUS_LEVEL: return "RES_SURPLUS_LEVEL";
    case FeatureId::RES_RELATIVE_WAGE: return "RES_RELATIVE_WAGE";
    }
    return "UNKNOWN";
}

static std::map<std::string, int> countFeatureUsage(const GPTree& tree) {
    std::map<std::string, int> counts;

    for (const auto& n : tree.nodes) {
        if (n.kind == NodeKind::FEATURE) {
            counts[featureIdToStringLocal(n.feat)]++;
        }
    }

    return counts;
}

static void writeFeatureCounts(std::ofstream& out, const std::map<std::string, int>& counts) {
    if (counts.empty()) {
        out << "NONE";
        return;
    }

    bool first = true;
    for (const auto& kv : counts) {
        if (!first) out << ", ";
        out << kv.first << "=" << kv.second;
        first = false;
    }
}

static const GP_Individual* bestMakespanPtr(const std::vector<GP_Individual>& xs) {
    if (xs.empty()) return nullptr;

    const GP_Individual* best = &xs[0];
    for (const auto& x : xs) {
        if (x.makespan < best->makespan ||
            (x.makespan == best->makespan && x.cost < best->cost)) {
            best = &x;
        }
    }
    return best;
}

static const GP_Individual* cheapestPtr(const std::vector<GP_Individual>& xs) {
    if (xs.empty()) return nullptr;

    const GP_Individual* best = &xs[0];
    for (const auto& x : xs) {
        if (x.cost < best->cost ||
            (std::abs(x.cost - best->cost) < 1e-9 && x.makespan < best->makespan)) {
            best = &x;
        }
    }
    return best;
}

static const GP_Individual* bestCostUnderMakespanPtr(const std::vector<GP_Individual>& xs, int maxMakespan) {
    const GP_Individual* best = nullptr;

    for (const auto& x : xs) {
        if (x.makespan <= maxMakespan) {
            if (best == nullptr ||
                x.cost < best->cost ||
                (std::abs(x.cost - best->cost) < 1e-9 && x.makespan < best->makespan)) {
                best = &x;
            }
        }
    }

    return best;
}

static void dumpIndividualBlock(std::ofstream& out, const std::string& label, const GP_Individual* ind) {
    out << "===== " << label << " =====\n";

    if (ind == nullptr) {
        out << "NONE\n\n";
        return;
    }

    out << "makespan=" << ind->makespan
        << ", cost=" << ind->cost
        << ", fitness=" << ind->fitness
        << ", msNorm=" << ind->msNorm
        << ", costNorm=" << ind->costNorm
        << "\n";

    out << "taskTreeDepth=" << ind->taskTree.depth()
        << ", taskTreeNodes=" << ind->taskTree.nodeCount()
        << ", resTreeDepth=" << ind->resTree.depth()
        << ", resTreeNodes=" << ind->resTree.nodeCount()
        << "\n";

    out << "taskTree=" << ind->taskTree.toString() << "\n";
    out << "resTree=" << ind->resTree.toString() << "\n";

    out << "taskFeatureCounts=";
    writeFeatureCounts(out, countFeatureUsage(ind->taskTree));
    out << "\n";

    out << "resFeatureCounts=";
    writeFeatureCounts(out, countFeatureUsage(ind->resTree));
    out << "\n\n";
}

static void writeArchiveDump(const GPEA_Params& P, const std::vector<GP_Individual>& archive) {
    if (!P.logGenerations) return;

    std::ofstream out(makeArchiveDumpPath(P), std::ofstream::out | std::ofstream::trunc);
    if (!out.is_open()) return;

    out << std::fixed << std::setprecision(6);

    out << "archive_size=" << archive.size() << "\n";
    out << "archive_count_le_600=" << countUnderMakespan(archive, 600) << "\n";
    out << "archive_count_le_700=" << countUnderMakespan(archive, 700) << "\n";
    out << "archive_count_le_800=" << countUnderMakespan(archive, 800) << "\n";
    out << "archive_count_le_1000=" << countUnderMakespan(archive, 1000) << "\n";
    out << "archive_count_le_1300=" << countUnderMakespan(archive, 1300) << "\n\n";

    dumpIndividualBlock(out, "BEST_MAKESPAN", bestMakespanPtr(archive));
    dumpIndividualBlock(out, "BEST_COST_UNDER_700", bestCostUnderMakespanPtr(archive, 700));
    dumpIndividualBlock(out, "BEST_COST_UNDER_800", bestCostUnderMakespanPtr(archive, 800));
    dumpIndividualBlock(out, "CHEAPEST_OVERALL", cheapestPtr(archive));
}

TreeEA::TreeEA(Instance& I, const GPEA_Params& params, CScheduler* imopseScheduler, bool isTAProblem)
    : inst(I),
    workInst_(I),
    P(params),
    rng(static_cast<unsigned>(P.seed)),
    imopseSch_(imopseScheduler),
    imopseIsTA_(isTAProblem) {
    gp::buildCPM(inst, cpm);
    gp::setCPMPrecalc(&cpm);
    bounds = compute_imopse_bounds(inst);
}


TreeEA::~TreeEA() {
    gp::setCPMPrecalc(nullptr);
}

void TreeEA::setSeedTrees(const GPTree& task, const GPTree& res) {
    seedTask_ = task;
    seedRes_ = res;
    hasSeed_ = true;
}

double TreeEA::rand01() {
    std::uniform_real_distribution<double> U(0.0, 1.0);
    return U(rng);
}
int TreeEA::randInt(int lo, int hi) {
    std::uniform_int_distribution<int> U(lo, hi);
    return U(rng);
}

Instance& TreeEA::resetWorkingInstance() const {
    for (auto& t : workInst_.tasks) {
        t.start = -1;
        t.finish = -1;
        t.assignedResources.clear();
    }

    for (auto& r : workInst_.resources) {
        r.busy = false;
        r.busyUntil = 0;
        r.busyStart = 0;
        r.totalBusy = 0;
    }

    return workInst_;
}

GP_Individual TreeEA::evaluate(const GP_Individual& src) const {
    GP_Individual ind = src;
    Instance& Ic = resetWorkingInstance();
    GPTreeRule    ruleT(ind.taskTree);
    GPTreeResRule ruleR(ind.resTree);

    const bool leanDecode = (P.useImopseEvaluate && imopseSch_ && imopseIsTA_);

    ScheduleOptions schedOpt;
    schedOpt.computeObjectiveStats = !leanDecode;
    schedOpt.keepTaskAssignedResources = !leanDecode;
    schedOpt.captureAssignedResByImopse = leanDecode;

    auto sim = Scheduler::withResources(Ic, ruleT, &ruleR, schedOpt);

    int ms = sim.makespan;
    double cost = sim.totalCost;

    if (P.useImopseEvaluate && imopseSch_ && imopseIsTA_) {
        CScheduler& sch = *imopseSch_;
        const auto& tasks = sch.GetTasks();
        const size_t n = tasks.size();

        auto pickCheapestCapable = [&](size_t taskIdx) -> TResourceID {
            std::vector<TResourceID> cap;
            sch.GetCapableResources(tasks[taskIdx], cap);
            if (cap.empty()) return (TResourceID)1;

            TResourceID best = cap[0];
            float bestSal = sch.GetResourceById(best)->GetSalary();
            for (size_t k = 1; k < cap.size(); ++k) {
                float s = sch.GetResourceById(cap[k])->GetSalary();
                if (s < bestSal) { bestSal = s; best = cap[k]; }
            }
            return best;
            };

        sch.Reset();
        for (size_t i = 0; i < n; ++i) {
            int rid = (i < sim.assignedResByImopseTaskIndex.size()) ? sim.assignedResByImopseTaskIndex[i] : -1;
            TResourceID resId = (rid > 0) ? (TResourceID)rid : pickCheapestCapable(i);
            sch.Assign(i, resId);
        }
        sch.BuildTimestamps_TA();

        ms = (int)sch.EvaluateDuration();
        cost = (double)sch.EvaluateCost();

        const double msMin = (double)sch.GetMinDuration();
        const double msMax = (double)sch.GetMaxDuration();
        const double cMin = (double)sch.GetMinCost();
        const double cMax = (double)sch.GetMaxCost();

        ind.msNorm = (ms - msMin) / (msMax - msMin);
        ind.costNorm = (cost - cMin) / (cMax - cMin);
    }
    else {
        auto normVals = imopse_minmax_normalize(ms, cost, bounds);
        ind.msNorm = normVals.first;
        ind.costNorm = normVals.second;
    }

    ind.makespan = ms;
    ind.cost = cost;

    const double fitnessNorm = P.weight * ind.msNorm + (1.0 - P.weight) * ind.costNorm;
    const double fitnessRaw = P.weight * (double)ind.makespan + (1.0 - P.weight) * ind.cost;
    ind.fitness = P.useNormalization ? fitnessNorm : fitnessRaw;

    return ind;
}


void TreeEA::initPopulation(std::vector<GP_Individual>& pop) {
    pop.clear();
    pop.reserve(P.popSize);

    if (hasSeed_ && P.popSize > 0) {
        GP_Individual ind;
        ind.taskTree = seedTask_;
        ind.resTree = seedRes_;
        ind = evaluate(ind);
        pop.push_back(ind);
    }

    while (pop.size() < P.popSize) {
        GP_Individual ind;
        ind.taskTree = GPTree::RandomTreeMS(rng, P.maxDepth);
        ind.resTree = GPTree::RandomTreeRES(rng, P.maxDepth);
        ind = evaluate(ind);
        pop.push_back(ind);
    }
}



const GP_Individual& TreeEA::tournament(const std::vector<GP_Individual>& pop, int k) {
    int best = -1;
    for (int i = 0; i < k; ++i) {
        int j = randInt(0, (int)pop.size() - 1);
        if (best == -1 || pop[j].fitness < pop[best].fitness) best = j;
    }
    return pop[best];
}

int TreeEA::pickRandomNode(const GPTree& t) {
    if (t.nodes.empty()) return -1;
    return randInt(0, (int)t.nodes.size() - 1);
}

static int depthOf(const GPTree& tr, int idx) {
    if (idx < 0 || idx >= (int)tr.nodes.size()) return 0;
    const GPNode& n = tr.nodes[idx];
    if (n.kind == NodeKind::CONST || n.kind == NodeKind::FEATURE) return 1;
    if (n.kind == NodeKind::UNARY)  return 1 + depthOf(tr, n.left);
    if (n.kind == NodeKind::BINARY) return 1 + std::max(depthOf(tr, n.left), depthOf(tr, n.right));
    return 1;
}

void TreeEA::clampDepth(GPTree& t, int maxDepth, bool isResTree) {
    if (t.isEmpty()) return;

    if (isResTree) ++clampCallsRes_;
    else           ++clampCallsTask_;

    const int allowedDepth = maxDepth + 1;

    const int beforeDepth = t.depth();
    if (beforeDepth <= allowedDepth) return;

    const size_t beforeNodes = t.nodes.size();

    if (isResTree) {
        ++clampAppliedRes_;
        clampPrevDepthSumRes_ += (size_t)beforeDepth;
        clampPrevNodesSumRes_ += beforeNodes;
    }
    else {
        ++clampAppliedTask_;
        clampPrevDepthSumTask_ += (size_t)beforeDepth;
        clampPrevNodesSumTask_ += beforeNodes;
    }

    const auto& pool = isResTree ? resFeatPool() : taskFeatPool();

    auto makeLeaf = [&]() -> GPNode {
        GPNode leaf{};
        if (rand01() < 0.90) {
            leaf.kind = NodeKind::FEATURE;
            leaf.feat = pool[randInt(0, (int)pool.size() - 1)];
        }
        else {
            leaf.kind = NodeKind::CONST;
            std::uniform_real_distribution<double> U(-1.0, 1.0);
            leaf.constant = U(rng);
        }
        leaf.left = -1;
        leaf.right = -1;
        return leaf;
        };

    std::vector<int> q;
    q.reserve(t.nodes.size());

    std::vector<int> d(t.nodes.size(), -1);
    q.push_back(t.root);
    d[t.root] = 0;

    for (size_t i = 0; i < q.size(); ++i) {
        const int u = q[i];
        const int du = d[u];
        GPNode& n = t.nodes[u];

        if (du >= allowedDepth - 1) {
            if (n.kind == NodeKind::UNARY || n.kind == NodeKind::BINARY) {
                n = makeLeaf();
            }
            continue;
        }

        auto push = [&](int v) {
            if (v >= 0 && v < (int)t.nodes.size() && d[v] == -1) {
                d[v] = du + 1;
                q.push_back(v);
            }
            };

        if (n.kind == NodeKind::UNARY) {
            push(n.left);
        }
        else if (n.kind == NodeKind::BINARY) {
            push(n.left);
            push(n.right);
        }
    }

    t = t.extractSubtree(t.root);

    if (!t.hasAnyFeature()) {
        GPNode leaf{};
        leaf.kind = NodeKind::FEATURE;
        leaf.feat = pool[randInt(0, (int)pool.size() - 1)];
        leaf.left = leaf.right = -1;
        t.nodes.clear();
        t.nodes.push_back(leaf);
        t.root = 0;
    }

    if (t.depth() > allowedDepth) {
        GPNode leaf = makeLeaf();
        t.nodes.clear();
        t.nodes.push_back(leaf);
        t.root = 0;
    }
}

static int cloneSubtree(const GPTree& src, int idx, GPTree& dst) {
    if (idx < 0) return -1;
    const GPNode& sn = src.nodes[idx];
    GPNode dn = sn;
    dn.left = -1;
    dn.right = -1;

    int here = (int)dst.nodes.size();
    dst.nodes.push_back(dn);

    if (sn.kind == NodeKind::UNARY) {
        int L = cloneSubtree(src, sn.left, dst);
        dst.nodes[here].left = L;
    }
    else if (sn.kind == NodeKind::BINARY) {
        int L = cloneSubtree(src, sn.left, dst);
        int R = cloneSubtree(src, sn.right, dst);
        dst.nodes[here].left = L;
        dst.nodes[here].right = R;
    }
    return here;
}

static void replaceLeftChildAtRoot(GPTree& t, int newLeft) {
    if (t.root < 0 || t.root >= (int)t.nodes.size()) return;
    if (t.nodes[t.root].kind != NodeKind::BINARY) return;
    t.nodes[t.root].left = newLeft;
}


void TreeEA::crossover(GPTree& a, GPTree& b, bool isResTree) {
    if (a.isEmpty() || b.isEmpty()) return;

    GPTree A0 = a, B0 = b;

    int ia = pickRandomNode(A0);
    int ib = pickRandomNode(B0);
    if (ia < 0 || ib < 0) return;

    GPTree subA = A0.extractSubtree(ia);
    GPTree subB = B0.extractSubtree(ib);

    clampDepth(subA, P.maxDepth, isResTree);
    clampDepth(subB, P.maxDepth, isResTree);

    GPTree aChild = A0.graftedWith(ia, subB);
    GPTree bChild = B0.graftedWith(ib, subA);

    clampDepth(aChild, P.maxDepth, isResTree);
    clampDepth(bChild, P.maxDepth, isResTree);

    auto ok = [&](const GPTree& t) {
        return !t.isEmpty() && t.nodeCount() <= 100000 && t.isStructurallySound();
        };

    if (ok(aChild) && ok(bChild)) {
        a = std::move(aChild);
        b = std::move(bChild);
    }
    else {
        a = std::move(A0);
        b = std::move(B0);
    }
}


void TreeEA::mutateParam(GPTree& t, bool isResTree) {
    if (t.nodes.empty()) return;
    int i = pickRandomNode(t);
    if (i < 0) return;

    auto& n = t.nodes[i];
    const auto& pool = isResTree ? resFeatPool() : taskFeatPool();

    switch (n.kind) {
    case NodeKind::CONST: {
        std::normal_distribution<double> N(0.0, 1.0);
        n.constant += N(rng);
        break;
    }
    case NodeKind::FEATURE: {
        n.feat = pool[randInt(0, (int)pool.size() - 1)];
        break;
    }
    case NodeKind::UNARY:
        n.uop = (rand01() < 0.5 ? UnaryOp::NEG : UnaryOp::ABS);
        break;
    case NodeKind::BINARY: {
        static const BinaryOp ops[] = {
            BinaryOp::ADD,
            BinaryOp::SUB,
            BinaryOp::MUL,
            BinaryOp::MIN,
            BinaryOp::MAX
        };
        n.bop = ops[randInt(0, 4)];
        break;
    }
    }
}

void TreeEA::mutateStruct(GPTree& t, bool isResTree) {
    if (t.nodes.empty()) return;
    int i = pickRandomNode(t);
    if (i < 0) return;

    const auto& pool = isResTree ? resFeatPool() : taskFeatPool();

    double r = rand01();
    if (r < 0.25) {
        auto& n = t.nodes[i];
        n.kind = NodeKind::FEATURE;
        n.left = n.right = -1;
        n.feat = pool[randInt(0, (int)pool.size() - 1)];
    }
    else if (r < 0.50) {
        auto& n = t.nodes[i];
        n.kind = NodeKind::CONST;
        n.left = n.right = -1;
        std::uniform_real_distribution<double> U(-5.0, 5.0);
        n.constant = U(rng);
    }
    else if (r < 0.75) {
        t.nodes[i].kind = NodeKind::UNARY;
        t.nodes[i].uop = (rand01() < 0.5 ? UnaryOp::NEG : UnaryOp::ABS);

        GPNode C{};
        if (rand01() < 0.5) { C.kind = NodeKind::FEATURE; C.feat = pool[randInt(0, (int)pool.size() - 1)]; }
        else { C.kind = NodeKind::CONST; C.constant = 0.0; }
        C.left = C.right = -1;

        int childIdx = (int)t.nodes.size();
        t.nodes.push_back(C);

        t.nodes[i].left = childIdx;
        t.nodes[i].right = -1;
    }
    else {
        t.nodes.reserve(t.nodes.size() + 2);

        t.nodes[i].kind = NodeKind::BINARY;
        {
            static const BinaryOp ops[] = {
                BinaryOp::ADD,
                BinaryOp::SUB,
                BinaryOp::MUL,
                BinaryOp::MIN,
                BinaryOp::MAX
            };
            t.nodes[i].bop = ops[randInt(0, 4)];
        }

        GPNode L{}, R{};
        if (rand01() < 0.5) { L.kind = NodeKind::FEATURE; L.feat = pool[randInt(0, (int)pool.size() - 1)]; }
        else { L.kind = NodeKind::CONST;   L.constant = 0.0; }

        if (rand01() < 0.5) { R.kind = NodeKind::FEATURE; R.feat = pool[randInt(0, (int)pool.size() - 1)]; }
        else { R.kind = NodeKind::CONST;   R.constant = 0.0; }

        int leftIdx = (int)t.nodes.size();  t.nodes.push_back(L);
        int rightIdx = (int)t.nodes.size(); t.nodes.push_back(R);

        t.nodes[i].left = leftIdx;
        t.nodes[i].right = rightIdx;
    }

    clampDepth(t, P.maxDepth, isResTree);
}

void TreeEA::mutateMacroSubtreeReplace(GPTree& t, bool isResTree) {
    if (t.isEmpty()) return;

    if (isResTree) ++macroSubtreeAppliedRes_;
    else           ++macroSubtreeAppliedTask_;

    const int replaceIdx = pickRandomNode(t);
    if (replaceIdx < 0) return;

    const int minD = std::max(2, P.maxDepth / 2);
    const int maxD = std::max(2, P.maxDepth);
    const int regrowDepth = randInt(minD, maxD);

    GPTree donor = isResTree
        ? GPTree::RandomTreeRES(rng, regrowDepth)
        : GPTree::RandomTreeMS(rng, regrowDepth);

    clampDepth(donor, P.maxDepth, isResTree);

    GPTree base = t;
    GPTree child = base.graftedWith(replaceIdx, donor);
    clampDepth(child, P.maxDepth, isResTree);

    auto ok = [&](const GPTree& tr) {
        return !tr.isEmpty() && tr.nodeCount() <= 100000 && tr.isStructurallySound();
        };

    if (ok(child)) {
        t = std::move(child);
    }
    else {
        t = std::move(base);
    }
}

void TreeEA::updatePareto(const GP_Individual& ind) {
    GP_ParetoPoint p;
    p.makespan = ind.makespan;
    p.cost = ind.cost;
    p.msNorm = ind.msNorm;
    p.costNorm = ind.costNorm;

    for (const auto& q : pareto_) {
        if (dominates(q, p) || samePoint(q, p)) return;
    }

    pareto_.erase(
        std::remove_if(pareto_.begin(), pareto_.end(),
            [&](const GP_ParetoPoint& q) { return dominates(p, q); }),
        pareto_.end()
    );

    pareto_.push_back(p);
}

bool TreeEA::dominatesMO(const GP_Individual& a, const GP_Individual& b) const {
    const bool noWorse = (a.makespan <= b.makespan) && (a.cost <= b.cost);
    const bool strictlyBetter = (a.makespan < b.makespan) || (a.cost < b.cost);
    return noWorse && strictlyBetter;
}

static bool sameObj(const GP_Individual& a, const GP_Individual& b) {
    return (a.makespan == b.makespan) && (std::abs(a.cost - b.cost) < 1e-9);
}


static bool isDominatedByNorm(const GP_Individual& self, const GP_Individual& other)
{
    if (self.msNorm < other.msNorm) return false;
    if (self.costNorm < other.costNorm) return false;

    if (other.msNorm < self.msNorm) return true;
    if (other.costNorm < self.costNorm) return true;

    return false;
}

static bool isDuplicateEvalValueNorm(const GP_Individual& a, const GP_Individual& b)
{
    return (a.msNorm == b.msNorm) && (a.costNorm == b.costNorm);
}

void TreeEA::copyToArchiveWithFiltering(const std::vector<GP_Individual>& individuals)
{
    std::vector<const GP_Individual*> filteredIndividuals;
    filteredIndividuals.reserve(individuals.size());

    for (size_t p = 0; p < individuals.size(); ++p)
    {
        const GP_Individual* newInd = &individuals[p];
        bool isDominated = false;

        size_t i = 0;
        while (!isDominated && i < individuals.size())
        {
            if (p != i)
            {
                isDominated = isDominatedByNorm(*newInd, individuals[i]);
                if (!isDominated && p < i)
                {
                    isDominated = isDuplicateEvalValueNorm(*newInd, individuals[i]);
                }
            }
            ++i;
        }

        i = 0;
        while (!isDominated && i < archive_.size())
        {
            isDominated = isDominatedByNorm(*newInd, archive_[i]);
            if (!isDominated)
            {
                isDominated = isDuplicateEvalValueNorm(*newInd, archive_[i]);
            }
            ++i;
        }

        if (!isDominated)
        {
            filteredIndividuals.push_back(newInd);
        }
    }

    archive_.erase(std::remove_if(archive_.begin(), archive_.end(),
        [&](const GP_Individual& ind)
        {
            for (const GP_Individual* filteredInd : filteredIndividuals)
            {
                if (isDominatedByNorm(ind, *filteredInd))
                {
                    return true;
                }
            }
            return false;
        }),
        archive_.end());

    archive_.reserve(archive_.size() + filteredIndividuals.size());
    for (const GP_Individual* filteredInd : filteredIndividuals)
    {
        GP_Individual copy = *filteredInd;
        copy.selectedCount = 0;
        archive_.push_back(std::move(copy));
    }
}

static void copyToArchiveWithFilteringLogged(
    std::vector<GP_Individual>& archive,
    const std::vector<LoggedChild>& children,
    GenLogArchiveEntryCounts& stats)
{
    std::vector<const LoggedChild*> filteredChildren;
    filteredChildren.reserve(children.size());

    for (size_t p = 0; p < children.size(); ++p)
    {
        const LoggedChild* cand = &children[p];
        const GP_Individual* newInd = &cand->ind;
        bool isDominated = false;

        size_t i = 0;
        while (!isDominated && i < children.size())
        {
            if (p != i)
            {
                isDominated = isDominatedByNorm(*newInd, children[i].ind);
                if (!isDominated && p < i)
                {
                    isDominated = isDuplicateEvalValueNorm(*newInd, children[i].ind);
                }
            }
            ++i;
        }

        i = 0;
        while (!isDominated && i < archive.size())
        {
            isDominated = isDominatedByNorm(*newInd, archive[i]);
            if (!isDominated)
            {
                isDominated = isDuplicateEvalValueNorm(*newInd, archive[i]);
            }
            ++i;
        }

        if (!isDominated)
        {
            filteredChildren.push_back(cand);
        }
    }

    archive.erase(std::remove_if(archive.begin(), archive.end(),
        [&](const GP_Individual& ind)
        {
            for (const LoggedChild* child : filteredChildren)
            {
                if (isDominatedByNorm(ind, child->ind))
                {
                    return true;
                }
            }
            return false;
        }),
        archive.end());

    archive.reserve(archive.size() + filteredChildren.size());

    for (const LoggedChild* child : filteredChildren)
    {
        GP_Individual copy = child->ind;
        copy.selectedCount = 0;
        archive.push_back(std::move(copy));

        ++stats.totalEntries;
        if (child->ops.crossover) ++stats.entriesByCrossover;
        if (child->ops.paramMut) ++stats.entriesByParamMut;
        if (child->ops.structMut) ++stats.entriesByStructMut;
        if (child->ops.macroMut) ++stats.entriesByMacroMut;
        if (child->ind.makespan <= 700) ++stats.entriesLe700;
        if (child->ind.makespan <= 800) ++stats.entriesLe800;
    }
}

double TreeEA::objNorm(const GP_Individual& x, int objId) const {
    return (objId == 0) ? x.msNorm : x.costNorm;
}


std::vector<std::pair<int, int>> TreeEA::selectParentsBNTGA(int objectiveNumber, int populationSize)
{
    std::vector<std::pair<int, int>> selectedParents;

    if (archive_.size() < 2)
    {
        if (!archive_.empty()) selectedParents.emplace_back(0, 0);
        return selectedParents;
    }

    const int objectiveId = randInt(0, objectiveNumber - 1);

    std::sort(archive_.begin(), archive_.end(),
        [&](const GP_Individual& a, const GP_Individual& b)
        {
            return objNorm(a, objectiveId) < objNorm(b, objectiveId);
        });

    const size_t n = archive_.size();
    std::vector<double> gapValues(n, 0.0);

    gapValues[0] = std::numeric_limits<double>::max();
    gapValues[n - 1] = std::numeric_limits<double>::max();

    for (size_t i = 1; i < n - 1; ++i)
    {
        const double iValue = objNorm(archive_[i], objectiveId);
        gapValues[i] = std::max(iValue - objNorm(archive_[i - 1], objectiveId),
            objNorm(archive_[i + 1], objectiveId) - iValue);
    }

    for (size_t i = 0; i < n; ++i)
    {
        gapValues[i] = gapValues[i] / (double)(archive_[i].selectedCount + 1);
    }

    auto selectParentIdxByTournament = [&]() -> int
        {
            int parentIdx = randInt(0, (int)n - 1);
            double bestGap = gapValues[(size_t)parentIdx];
            for (int i = 1; i < P.tournamentK; ++i)
            {
                int randomIdx = randInt(0, (int)n - 1);
                if (gapValues[(size_t)randomIdx] > bestGap)
                {
                    bestGap = gapValues[(size_t)randomIdx];
                    parentIdx = randomIdx;
                }
            }
            return parentIdx;
        };

    selectedParents.reserve((size_t)populationSize / 2);
    for (int i = 0; i < populationSize; i += 2)
    {
        const int firstParentIdx = selectParentIdxByTournament();

        int secondParentIdx = 0;
        if (firstParentIdx == 0)
        {
            secondParentIdx = 1;
        }
        else if (firstParentIdx == (int)n - 1)
        {
            secondParentIdx = (int)n - 2;
        }
        else
        {
            secondParentIdx = firstParentIdx + (randInt(0, 1) == 0 ? 1 : -1);
        }

        archive_[(size_t)firstParentIdx].selectedCount += 1;
        archive_[(size_t)secondParentIdx].selectedCount += 1;

        selectedParents.emplace_back(firstParentIdx, secondParentIdx);
    }

    return selectedParents;
}

std::vector<std::vector<int>> TreeEA::nonDominatedSort(std::vector<GP_Individual>& pop) const
{

    auto dominatesNorm = [](const GP_Individual& a, const GP_Individual& b) -> bool
        {
            if (b.msNorm < a.msNorm)   return false;
            if (b.costNorm < a.costNorm) return false;

            if (a.msNorm < b.msNorm)   return true;
            if (a.costNorm < b.costNorm) return true;

            return false;
        };

    const int N = (int)pop.size();
    std::vector<std::vector<int>> S(N);
    std::vector<int> n(N, 0);
    std::vector<std::vector<int>> fronts;

    fronts.push_back({});

    for (int p = 0; p < N; ++p) {
        S[p].clear();
        n[p] = 0;

        for (int q = 0; q < N; ++q) {
            if (p == q) continue;

            if (dominatesNorm(pop[p], pop[q])) {
                S[p].push_back(q);
            }
            else if (dominatesNorm(pop[q], pop[p])) {
                n[p]++;
            }
        }

        if (n[p] == 0) {
            pop[p].rank = 0;
            fronts[0].push_back(p);
        }
    }

    int i = 0;
    while (i < (int)fronts.size() && !fronts[i].empty()) {
        std::vector<int> Q;
        for (int p : fronts[i]) {
            for (int q : S[p]) {
                n[q]--;
                if (n[q] == 0) {
                    pop[q].rank = i + 1;
                    Q.push_back(q);
                }
            }
        }
        i++;
        if (!Q.empty()) fronts.push_back(std::move(Q));
        else break;
    }

    return fronts;
}

void TreeEA::calcCrowdingDistance(std::vector<GP_Individual>& pop, const std::vector<int>& front) const
{
    if (front.empty()) return;

    for (int idx : front) pop[idx].crowding = 0.0;

    auto setInfEnds = [&](auto getter) {
        std::vector<int> idx = front;
        std::sort(idx.begin(), idx.end(), [&](int a, int b) { return getter(pop[a]) < getter(pop[b]); });

        pop[idx.front()].crowding = std::numeric_limits<double>::max();
        pop[idx.back()].crowding = std::numeric_limits<double>::max();

        for (int i = 1; i < (int)idx.size() - 1; ++i) {
            if (!std::isfinite(pop[idx[i]].crowding)) continue;
            const double plus = getter(pop[idx[i + 1]]);
            const double minus = getter(pop[idx[i - 1]]);
            pop[idx[i]].crowding += (plus - minus);
        }
        };

    setInfEnds([](const GP_Individual& x) { return x.msNorm; });
    setInfEnds([](const GP_Individual& x) { return x.costNorm; });
}

const GP_Individual& TreeEA::tournamentMO(const std::vector<GP_Individual>& pop, int k)
{
    int best = randInt(0, (int)pop.size() - 1);
    int bestRank = pop[best].rank;

    for (int i = 1; i < k; ++i) {
        int j = randInt(0, (int)pop.size() - 1);
        int r = pop[j].rank;
        if (r < bestRank) {
            bestRank = r;
            best = j;
        }
    }
    return pop[best];
}

std::vector<GP_Individual> TreeEA::selectNextPopulationNSGA2(std::vector<GP_Individual>& combined)
{
    auto fronts = nonDominatedSort(combined);

    std::vector<GP_Individual> next;
    next.reserve(P.popSize);

    for (const auto& f : fronts) {
        if (f.empty()) break;

        if (next.size() + f.size() > P.popSize) {
            calcCrowdingDistance(combined, f);

            std::vector<int> tmp = f;
            std::sort(tmp.begin(), tmp.end(), [&](int a, int b) {
                return combined[a].crowding < combined[b].crowding; 
                });

            for (int idx : tmp) {
                next.push_back(combined[idx]);
                if (next.size() >= P.popSize) break;
            }
            break;
        }
        else {
            for (int idx : f) next.push_back(combined[idx]);
        }
    }

    return next;
}

GP_Individual TreeEA::run() {
    std::vector<GP_Individual> pop;
    initPopulation(pop);

    writeGenerationLogHeader(P);

    if (P.useBNTGA) {
        archive_.clear();
        archive_.reserve(P.popSize * 2);

        copyToArchiveWithFiltering(pop);

        histBest_.clear();
        histAvg_.clear();
        histWorst_.clear();

        auto bestIt0 = std::min_element(pop.begin(), pop.end(),
            [](const GP_Individual& a, const GP_Individual& b) { return a.fitness < b.fitness; });

        GP_Individual bestSoFar = *bestIt0;

        auto worstIt0 = std::max_element(pop.begin(), pop.end(),
            [](const GP_Individual& a, const GP_Individual& b) { return a.fitness < b.fitness; });

        double sum0 = std::accumulate(pop.begin(), pop.end(), 0.0,
            [](double acc, const GP_Individual& x) { return acc + x.fitness; });

        double avg0 = sum0 / std::max<size_t>(1, pop.size());

        histBest_.push_back(bestIt0->fitness);
        histAvg_.push_back(avg0);
        histWorst_.push_back(worstIt0->fitness);

        bestGen0_ = *bestIt0;
        hasBestGen0_ = true;

        GenLogArchiveEntryCounts gen0ArchiveOps{};
        gen0ArchiveOps.totalEntries = (int)archive_.size();
        gen0ArchiveOps.entriesLe700 = countUnderMakespan(archive_, 700);
        gen0ArchiveOps.entriesLe800 = countUnderMakespan(archive_, 800);

        appendGenerationLog(
            P,
            0,
            pop,
            archive_.size(),
            (long long)archive_.size(),
            GenLogOpCounts{},
            gen0ArchiveOps
        );

        for (size_t gen = 0; gen < P.generations; ++gen) {

            std::vector<GP_Individual> offspring;
            offspring.reserve(P.popSize);

            std::vector<LoggedChild> loggedOffspring;
            loggedOffspring.reserve(P.popSize);

            GenLogOpCounts genOps{};
            const size_t archiveBefore = archive_.size();

            auto pairs = selectParentsBNTGA(/*objectiveNumber=*/2, (int)P.popSize);

            const std::vector<GP_Individual> archiveSnap = archive_;

            for (auto [ia, ib] : pairs) {

                if (ia < 0 || ib < 0 || ia >= (int)archiveSnap.size() || ib >= (int)archiveSnap.size()) {
                    continue;
                }

                GP_Individual c1 = archiveSnap[ia];
                GP_Individual c2 = archiveSnap[ib];

                ChildOpFlags op1{}, op2{};

                if (rand01() < P.pCrossover) {
                    crossover(c1.taskTree, c2.taskTree, false);
                    op1.crossover = true;
                    op2.crossover = true;
                }
                if (rand01() < P.pCrossover) {
                    crossover(c1.resTree, c2.resTree, true);
                    op1.crossover = true;
                    op2.crossover = true;
                }

                if (rand01() < P.pMutParam) {
                    mutateParam(c1.taskTree, false);
                    op1.paramMut = true;
                }
                if (rand01() < P.pMutParam) {
                    mutateParam(c2.taskTree, false);
                    op2.paramMut = true;
                }
                if (rand01() < P.pMutParam) {
                    mutateParam(c1.resTree, true);
                    op1.paramMut = true;
                }
                if (rand01() < P.pMutParam) {
                    mutateParam(c2.resTree, true);
                    op2.paramMut = true;
                }

                auto structOrMacro = [&](GPTree& tr, bool isRes, ChildOpFlags& opFlags) {
                    if (rand01() < P.pMutMacroSubtree) {
                        mutateMacroSubtreeReplace(tr, isRes);
                        opFlags.macroMut = true;
                    }
                    else if (rand01() < P.pMutStruct) {
                        mutateStruct(tr, isRes);
                        opFlags.structMut = true;
                    }
                    };

                structOrMacro(c1.taskTree, false, op1);
                structOrMacro(c2.taskTree, false, op2);
                structOrMacro(c1.resTree, true, op1);
                structOrMacro(c2.resTree, true, op2);

                auto e1 = evaluate(c1);
                if (op1.crossover) ++genOps.crossoverChildren;
                if (op1.paramMut) ++genOps.paramMutChildren;
                if (op1.structMut) ++genOps.structMutChildren;
                if (op1.macroMut) ++genOps.macroMutChildren;
                offspring.push_back(e1);
                loggedOffspring.push_back(LoggedChild{ e1, op1 });

                if (offspring.size() < P.popSize) {
                    auto e2 = evaluate(c2);
                    if (op2.crossover) ++genOps.crossoverChildren;
                    if (op2.paramMut) ++genOps.paramMutChildren;
                    if (op2.structMut) ++genOps.structMutChildren;
                    if (op2.macroMut) ++genOps.macroMutChildren;
                    offspring.push_back(e2);
                    loggedOffspring.push_back(LoggedChild{ e2, op2 });
                }
            }

            GenLogArchiveEntryCounts archiveOps{};
            copyToArchiveWithFilteringLogged(archive_, loggedOffspring, archiveOps);

            pop.swap(offspring);

            auto itBest = std::min_element(pop.begin(), pop.end(),
                [](const GP_Individual& a, const GP_Individual& b) { return a.fitness < b.fitness; });

            auto itWorst = std::max_element(pop.begin(), pop.end(),
                [](const GP_Individual& a, const GP_Individual& b) { return a.fitness < b.fitness; });

            double sum = std::accumulate(pop.begin(), pop.end(), 0.0,
                [](double acc, const GP_Individual& x) { return acc + x.fitness; });

            double avg = sum / std::max<size_t>(1, pop.size());

            histBest_.push_back(itBest->fitness);
            histAvg_.push_back(avg);
            histWorst_.push_back(itWorst->fitness);

            if (itBest->fitness < bestSoFar.fitness) bestSoFar = *itBest;

            const long long archiveDelta =
                (long long)archive_.size() - (long long)archiveBefore;

            appendGenerationLog(
                P,
                gen + 1,
                pop,
                archive_.size(),
                archiveDelta,
                genOps,
                archiveOps
            );

        }

        pareto_.clear();
        pareto_.reserve(archive_.size());
        for (const auto& x : archive_) {
            GP_ParetoPoint p;
            p.makespan = x.makespan;
            p.cost = x.cost;
            p.msNorm = x.msNorm;
            p.costNorm = x.costNorm;
            pareto_.push_back(p);
        }

        writeArchiveDump(P, archive_);

        auto writeTraceFor = [&](const std::string& fileName,
            const std::string& label,
            const GP_Individual* ind)
            {
                if (!P.logGenerations || ind == nullptr) return;

                std::ofstream out(makeSchedulerTracePath(P, fileName), std::ofstream::out | std::ofstream::trunc);
                if (!out.is_open()) return;

                out << std::fixed << std::setprecision(6);
                out << "TRACE_LABEL=" << label << "\n";
                out << "makespan=" << ind->makespan
                    << ", cost=" << ind->cost
                    << ", fitness=" << ind->fitness
                    << ", msNorm=" << ind->msNorm
                    << ", costNorm=" << ind->costNorm
                    << "\n";
                out << "taskTree=" << ind->taskTree.toString() << "\n";
                out << "resTree=" << ind->resTree.toString() << "\n\n";

                Instance& Itrace = resetWorkingInstance();
                GPTreeRule ruleT(ind->taskTree);
                GPTreeResRule ruleR(ind->resTree);

                ScheduleOptions traceOpt;
                traceOpt.computeObjectiveStats = true;
                traceOpt.keepTaskAssignedResources = true;
                traceOpt.captureAssignedResByImopse = false;

                bool oldTrace = g_trace;
                std::streambuf* oldBuf = std::cout.rdbuf(out.rdbuf());

                g_trace = true;
                auto traceRes = Scheduler::withResources(Itrace, ruleT, &ruleR, traceOpt);
                g_trace = oldTrace;
                std::cout.rdbuf(oldBuf);

                out << "\nTRACE_RESULT makespan=" << traceRes.makespan
                    << ", totalCost=" << traceRes.totalCost << "\n";
            };

        writeTraceFor(
            "gphh_trace_best_makespan.txt",
            "BEST_MAKESPAN",
            bestMakespanPtr(archive_)
        );

        writeTraceFor(
            "gphh_trace_best_cost_le_700.txt",
            "BEST_COST_LE_700",
            bestCostUnderMakespanPtr(archive_, 700)
        );

        return bestSoFar;
    }

        if (P.useNSGA2) {
            archive_.clear();
            archive_.reserve(P.popSize * 2);
            copyToArchiveWithFiltering(pop);

        }
        else {
            pareto_.clear();
            pareto_.reserve(P.popSize * 2);
            for (const auto& ind : pop) updatePareto(ind);

        }

    histBest_.clear();
    histAvg_.clear();
    histWorst_.clear();

    auto itBest0 = std::min_element(pop.begin(), pop.end(),
        [](const GP_Individual& a, const GP_Individual& b) { return a.fitness < b.fitness; });
    auto itWorst0 = std::max_element(pop.begin(), pop.end(),
        [](const GP_Individual& a, const GP_Individual& b) { return a.fitness < b.fitness; });
    double sum0 = std::accumulate(pop.begin(), pop.end(), 0.0,
        [](double acc, const GP_Individual& x) { return acc + x.fitness; });
    double avg0 = sum0 / std::max<size_t>(1, pop.size());

    histBest_.push_back(itBest0->fitness);
    histAvg_.push_back(avg0);
    histWorst_.push_back(itWorst0->fitness);

    bestGen0_ = *itBest0;
    hasBestGen0_ = true;

    appendGenerationLog(
        P,
        0,
        pop,
        P.useNSGA2 ? archive_.size() : pareto_.size(),
        (long long)(P.useNSGA2 ? archive_.size() : pareto_.size()),
        GenLogOpCounts{},
        GenLogArchiveEntryCounts{}
    );
    GP_Individual bestSoFar = bestGen0_;


    for (size_t gen = 0; gen < P.generations; ++gen) {

        std::vector<GP_Individual> offspring;
        offspring.reserve(P.popSize);

        GenLogOpCounts genOps{};
        const size_t archiveBefore = P.useNSGA2 ? archive_.size() : pareto_.size();

        while (offspring.size() < P.popSize) {

            const GP_Individual& p1 = P.useNSGA2 ? tournamentMO(pop, P.tournamentK)
                : tournament(pop, P.tournamentK);
            const GP_Individual& p2 = P.useNSGA2 ? tournamentMO(pop, P.tournamentK)
                : tournament(pop, P.tournamentK);

            GP_Individual c1 = p1;
            GP_Individual c2 = p2;

            ChildOpFlags op1{}, op2{};

            if (rand01() < P.pCrossover) {
                crossover(c1.taskTree, c2.taskTree, false);
                op1.crossover = true;
                op2.crossover = true;
            }
            if (rand01() < P.pCrossover) {
                crossover(c1.resTree, c2.resTree, true);
                op1.crossover = true;
                op2.crossover = true;
            }

            if (rand01() < P.pMutParam) {
                mutateParam(c1.taskTree, false);
                op1.paramMut = true;
            }
            if (rand01() < P.pMutParam) {
                mutateParam(c2.taskTree, false);
                op2.paramMut = true;
            }
            if (rand01() < P.pMutParam) {
                mutateParam(c1.resTree, true);
                op1.paramMut = true;
            }
            if (rand01() < P.pMutParam) {
                mutateParam(c2.resTree, true);
                op2.paramMut = true;
            }

            auto structOrMacro = [&](GPTree& tr, bool isRes, ChildOpFlags& opFlags) {
                if (rand01() < P.pMutMacroSubtree) {
                    mutateMacroSubtreeReplace(tr, isRes);
                    opFlags.macroMut = true;
                }
                else if (rand01() < P.pMutStruct) {
                    mutateStruct(tr, isRes);
                    opFlags.structMut = true;
                }
                };

            structOrMacro(c1.taskTree, false, op1);
            structOrMacro(c2.taskTree, false, op2);
            structOrMacro(c1.resTree, true, op1);
            structOrMacro(c2.resTree, true, op2);

            auto e1 = evaluate(c1);
            if (op1.crossover) ++genOps.crossoverChildren;
            if (op1.paramMut) ++genOps.paramMutChildren;
            if (op1.structMut) ++genOps.structMutChildren;
            if (op1.macroMut) ++genOps.macroMutChildren;

            if (!P.useNSGA2) updatePareto(e1);
            offspring.push_back(e1);

            if (offspring.size() < P.popSize) {
                auto e2 = evaluate(c2);
                if (op2.crossover) ++genOps.crossoverChildren;
                if (op2.paramMut) ++genOps.paramMutChildren;
                if (op2.structMut) ++genOps.structMutChildren;
                if (op2.macroMut) ++genOps.macroMutChildren;

                if (!P.useNSGA2) updatePareto(e2);
                offspring.push_back(e2);
            }
        }

        if (P.useNSGA2) {
            copyToArchiveWithFiltering(offspring);
            std::vector<GP_Individual> combined;
            combined.reserve(pop.size() + offspring.size());
            combined.insert(combined.end(), pop.begin(), pop.end());
            combined.insert(combined.end(), offspring.begin(), offspring.end());

            pop = selectNextPopulationNSGA2(combined);
        }
        else {
            std::vector<GP_Individual> next;
            next.reserve(pop.size());

            size_t E = std::min<size_t>(P.eliteCount, pop.size());
            if (E > 0) {
                std::vector<GP_Individual> tmp = pop;
                std::partial_sort(tmp.begin(), tmp.begin() + E, tmp.end(),
                    [](const GP_Individual& a, const GP_Individual& b) { return a.fitness < b.fitness; });
                for (size_t e = 0; e < E; ++e) next.push_back(tmp[e]);
            }

            for (size_t i = 0; i < offspring.size() && next.size() < pop.size(); ++i) {
                next.push_back(offspring[i]);
            }

            pop.swap(next);
        }

        auto itBest = std::min_element(pop.begin(), pop.end(),
            [](const GP_Individual& a, const GP_Individual& b) { return a.fitness < b.fitness; });
        auto itWorst = std::max_element(pop.begin(), pop.end(),
            [](const GP_Individual& a, const GP_Individual& b) { return a.fitness < b.fitness; });
        double sum = std::accumulate(pop.begin(), pop.end(), 0.0,
            [](double acc, const GP_Individual& x) { return acc + x.fitness; });
        double avg = sum / std::max<size_t>(1, pop.size());

        histBest_.push_back(itBest->fitness);
        histAvg_.push_back(avg);
        histWorst_.push_back(itWorst->fitness);

        if (itBest->fitness < bestSoFar.fitness) bestSoFar = *itBest;

        const size_t archiveAfter = P.useNSGA2 ? archive_.size() : pareto_.size();
        const long long archiveDelta =
            (long long)archiveAfter - (long long)archiveBefore;

        appendGenerationLog(
            P,
            gen + 1,
            pop,
            archiveAfter,
            archiveDelta,
            genOps,
            GenLogArchiveEntryCounts{}
        );

    }

    if (P.useNSGA2) {
        pareto_.clear();
        pareto_.reserve(archive_.size());
        for (const auto& x : archive_) {
            GP_ParetoPoint p;
            p.makespan = x.makespan;
            p.cost = x.cost;
            p.msNorm = x.msNorm;
            p.costNorm = x.costNorm;
            pareto_.push_back(p);
        }

        writeArchiveDump(P, archive_);
    }

    return bestSoFar;
}
