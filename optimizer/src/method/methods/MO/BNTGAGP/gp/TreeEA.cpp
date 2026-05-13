#include "TreeEA.hpp"
#include "../rules/GPTreeRule.hpp"
#include "../rules/GPTreeResRule.hpp"
#include "problem/problems/MSRCPSP/CScheduler.h"
#include "utils/logger/CExperimentLogger.h"

#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <unordered_map>

namespace {
    struct EvalCacheEntry {
        int makespan = 0;
        double cost = 0.0;
        double msNorm = 0.0;
        double costNorm = 0.0;
        double fitness = 0.0;
    };

    thread_local std::unordered_map<std::uint64_t, EvalCacheEntry> g_evalCache;

    static inline std::uint64_t mix64(std::uint64_t x) {
        x += 0x9e3779b97f4a7c15ULL;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        x = x ^ (x >> 31);
        return x;
    }

    static inline void hashCombine(std::uint64_t& seed, std::uint64_t value) {
        seed ^= mix64(value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2));
    }

    static inline std::uint64_t doubleBits(double value) {
        std::uint64_t bits = 0;
        static_assert(sizeof(bits) == sizeof(value), "double size mismatch");
        std::memcpy(&bits, &value, sizeof(bits));
        return bits;
    }

    static std::uint64_t buildTreeCacheHash(const GPTree& t) {
        std::uint64_t h = 0x6a09e667f3bcc909ULL;

        hashCombine(h, (std::uint64_t)(std::int64_t)t.root);
        hashCombine(h, (std::uint64_t)t.nodes.size());

        for (const auto& n : t.nodes) {
            hashCombine(h, (std::uint64_t)(int)n.kind);
            hashCombine(h, doubleBits(n.constant));
            hashCombine(h, (std::uint64_t)(int)n.feat);
            hashCombine(h, (std::uint64_t)(int)n.uop);
            hashCombine(h, (std::uint64_t)(int)n.bop);
            hashCombine(h, (std::uint64_t)(std::int64_t)n.left);
            hashCombine(h, (std::uint64_t)(std::int64_t)n.right);
        }

        return h;
    }

    static std::uint64_t buildIndividualCacheHash(
        const GP_Individual& ind,
        bool useSinglePairTree)
    {
        std::uint64_t h = 0x243f6a8885a308d3ULL;

        if (useSinglePairTree) {
            hashCombine(h, 0x5041495254524545ULL);
            hashCombine(h, buildTreeCacheHash(ind.taskTree));
            return h;
        }

        hashCombine(h, 0x5441534b54524545ULL);
        hashCombine(h, buildTreeCacheHash(ind.taskTree));
        hashCombine(h, 0x5245535452454521ULL);
        hashCombine(h, buildTreeCacheHash(ind.resTree));

        return h;
    }
    static const std::vector<FeatureId>& loggedFeatureOrder() {
        static const std::vector<FeatureId> v = GPTree::allPairFeatures();
        return v;
    }

    static const std::unordered_map<int, size_t>& loggedFeatureIndex() {
        static const std::unordered_map<int, size_t> idx = []() {
            std::unordered_map<int, size_t> m;
            const auto& feats = loggedFeatureOrder();
            m.reserve(feats.size());
            for (size_t i = 0; i < feats.size(); ++i) {
                m.emplace((int)feats[i], i);
            }
            return m;
            }();
        return idx;
    }

    static const char* featureShortName(FeatureId f) {
        switch (f) {
        case FeatureId::DURATION:                   return "DUR";
        case FeatureId::REQ_LEVEL:                  return "REQ";
        case FeatureId::AVAIL_SKILL:                return "AVAIL";
        case FeatureId::CRITLEN:                    return "CRITLEN";
        case FeatureId::SLACK:                      return "SLACK";
        case FeatureId::DESC_COUNT:                 return "DESC_COUNT";
        case FeatureId::TASK_CRITICAL_PRESSURE:     return "TASK_CRIT";
        case FeatureId::TASK_RELEASE_PRESSURE:      return "REL_PRESS";
        case FeatureId::AVAIL_GAP:                  return "GAP";
        case FeatureId::CHEAPEST_COST_NOW:          return "CHEAP";
        case FeatureId::COST_PER_SKILL_NOW:         return "CHEAP_PER_SK";
        case FeatureId::TASK_RES_COUNT:             return "TASK_RES";
        case FeatureId::AVG_RES_COST:               return "AVG_RES_COST";
        case FeatureId::UNSCHED_TASKS:              return "UNSCHED";
        case FeatureId::MIN_FEASIBLE_COST_NOW:      return "MIN_COST_NOW";
        case FeatureId::COST_REGRET_NOW:            return "REGRET_NOW";

        case FeatureId::RES_WAGE:                   return "RES_WAGE";
        case FeatureId::RES_SKILL_LEVEL:            return "RES_SKILL";
        case FeatureId::RES_IDLE_TIME:              return "RES_IDLE";
        case FeatureId::RES_CAN_START_NOW:          return "RES_CAN_NOW";
        case FeatureId::RES_UTILIZATION:            return "RES_UTIL";
        case FeatureId::RES_WAGE_PER_LEVEL:         return "RES_W_PER_L";
        case FeatureId::RES_ASSIGN_COST:            return "RES_ASSIGN_COST";
        case FeatureId::RES_ASSIGN_PREMIUM_ALL:     return "RES_PREMIUM";
        case FeatureId::RES_RESERVE_PRESSURE:       return "RES_RESERVE";
        case FeatureId::RES_FAMILY_MISMATCH:        return "RES_FAM_MIS";
        case FeatureId::RES_FUTURE_BRANCH_FIT:      return "RES_FUT_BRANCH";
        case FeatureId::RES_BOTTLENECK_PRESERVATION:return "RES_BOTTLENECK";
        case FeatureId::RES_SPECIALIST_MISUSE:      return "RES_SPEC_MIS";
        case FeatureId::RES_RELATIVE_WAGE:          return "RES_REL_WAGE";
        }
        return "?";
    }

    static void accumulateTreeFeatureCounts(
        const GPTree& t,
        std::vector<size_t>& counts,
        size_t& totalNodes)
    {
        totalNodes += t.nodes.size();

        const auto& featIndex = loggedFeatureIndex();

        for (const auto& n : t.nodes) {
            if (n.kind != NodeKind::FEATURE) {
                continue;
            }

            auto it = featIndex.find((int)n.feat);
            if (it != featIndex.end()) {
                counts[it->second] += 1;
            }
        }
    }

    static void writeNodeDistributionSnapshot(
        const std::vector<GP_Individual>& pool,
        size_t generation,
        bool useSinglePairTree,
        const char* sourceName,
        bool overwrite)
    {
        if (CExperimentLogger::m_OutputDataPathPrefix.empty()) {
            return;
        }

        const auto& feats = loggedFeatureOrder();
        std::vector<size_t> counts(feats.size(), 0);
        size_t totalNodes = 0;

        for (const auto& ind : pool) {
            accumulateTreeFeatureCounts(ind.taskTree, counts, totalNodes);

            if (!useSinglePairTree) {
                accumulateTreeFeatureCounts(ind.resTree, counts, totalNodes);
            }
        }

        const std::filesystem::path outPath =
            std::filesystem::path(CExperimentLogger::m_OutputDataPathPrefix) /
            ("node_distribution_" + std::string(sourceName) + ".csv");

        std::ofstream out(
            outPath,
            overwrite ? std::ios::out : (std::ios::out | std::ios::app));

        if (!out.is_open()) {
            return;
        }

        if (overwrite) {
            out << "generation,pool_size,total_nodes";
            for (FeatureId f : feats) {
                out << "," << featureShortName(f);
            }
            out << "\n";
        }

        out << generation
            << "," << pool.size()
            << "," << totalNodes;

        for (size_t c : counts) {
            out << "," << c;
        }
        out << "\n";
    }

}

static const std::vector<FeatureId>& taskFeatPool() {
    static const std::vector<FeatureId> v = GPTree::allTaskFeatures();
    return v;
}

static const std::vector<FeatureId>& resFeatPool() {
    static const std::vector<FeatureId> v = GPTree::allResFeatures();
    return v;
}

static const std::vector<FeatureId>& pairFeatPool() {
    static const std::vector<FeatureId> v = GPTree::allPairFeatures();
    return v;
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
    g_evalCache.clear();
    g_evalCache.reserve(P.popSize * 8);
}


TreeEA::~TreeEA() {
    gp::setCPMPrecalc(nullptr);
}

double TreeEA::rand01() {
    std::uniform_real_distribution<double> U(0.0, 1.0);
    return U(rng);
}
int TreeEA::randInt(int lo, int hi) {
    std::uniform_int_distribution<int> U(lo, hi);
    return U(rng);
}

Instance& TreeEA::resetWorkingInstance(bool clearAssignedResources) const {
    for (auto& t : workInst_.tasks) {
        t.start = -1;
        t.finish = -1;
        if (clearAssignedResources) {
            t.assignedResources.clear();
        }
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

    const std::uint64_t cacheKey = buildIndividualCacheHash(ind, P.useSinglePairTree);
    auto itCached = g_evalCache.find(cacheKey);
    if (itCached != g_evalCache.end()) {
        ind.makespan = itCached->second.makespan;
        ind.cost = itCached->second.cost;
        ind.msNorm = itCached->second.msNorm;
        ind.costNorm = itCached->second.costNorm;
        ind.fitness = itCached->second.fitness;
        return ind;
    }

    GPTreeRule gpTaskRule(ind.taskTree);
    GPTreeResRule gpResRule(ind.resTree);

    const IDispatchingRule& taskRule =
        static_cast<const IDispatchingRule&>(gpTaskRule);

    const GPTreeResRule* resRule = P.useSinglePairTree ? nullptr : &gpResRule;
    const GPTree* pairTree = P.useSinglePairTree ? &ind.taskTree : nullptr;

    const bool leanDecode = (imopseSch_ && imopseIsTA_);

    ScheduleOptions schedOpt;
    schedOpt.computeObjectiveStats = !leanDecode;
    schedOpt.keepTaskAssignedResources = !leanDecode;
    schedOpt.captureAssignedResByImopse = leanDecode;

    Instance& Ic = resetWorkingInstance(schedOpt.keepTaskAssignedResources);

    auto sim = Scheduler::withResources(Ic, taskRule, resRule, schedOpt, pairTree);

    int ms = sim.makespan;
    double cost = sim.totalCost;

    if (imopseSch_ && imopseIsTA_) {
        CScheduler& sch = *imopseSch_;
        const size_t n = sch.GetTasks().size();

        sch.Reset();
        for (size_t i = 0; i < n; ++i) {
            const TResourceID resId =
                (TResourceID)sim.assignedResByImopseTaskIndex.at(i);
            sch.Assign(i, resId);
        }
        sch.BuildTimestamps_TA();

        ms = (int)sch.EvaluateDuration();
        cost = (double)sch.EvaluateCost();

        const double msMin = (double)sch.GetMinDuration();
        const double msMax = (double)sch.GetMaxDuration();
        const double cMin = (double)sch.GetMinCost();
        const double cMax = (double)sch.GetMaxCost();

        ind.msNorm = (msMax > msMin)
            ? (ms - msMin) / (msMax - msMin)
            : 0.0;

        ind.costNorm = (cMax > cMin)
            ? (cost - cMin) / (cMax - cMin)
            : 0.0;
    }
    else {
        auto normVals = imopse_minmax_normalize(ms, cost, bounds);
        ind.msNorm = normVals.first;
        ind.costNorm = normVals.second;
    }

    ind.makespan = ms;
    ind.cost = cost;

    ind.fitness = ind.msNorm + ind.costNorm;

    g_evalCache.emplace(cacheKey, EvalCacheEntry{
        ind.makespan,
        ind.cost,
        ind.msNorm,
        ind.costNorm,
        ind.fitness
        });

    return ind;
}

void TreeEA::initPopulation(std::vector<GP_Individual>& pop) {
    pop.clear();
    pop.reserve(P.popSize);

    while (pop.size() < P.popSize) {
        GP_Individual ind;
        ind.taskTree = P.useSinglePairTree
            ? GPTree::RandomTreePAIR(rng, P.maxDepth)
            : GPTree::RandomTreeMS(rng, P.maxDepth);

        if (!P.useSinglePairTree) {
            ind.resTree = GPTree::RandomTreeRES(rng, P.maxDepth);
        }

        ind = evaluate(ind);
        pop.push_back(std::move(ind));
    }
}

int TreeEA::pickRandomNode(const GPTree& t) {
    if (t.nodes.empty()) return -1;
    return randInt(0, (int)t.nodes.size() - 1);
}

void TreeEA::clampDepth(GPTree& t, int maxDepth, bool isResTree) {
    if (t.isEmpty()) return;

    const int allowedDepth = maxDepth + 1;

    const int beforeDepth = t.depth();
    if (beforeDepth <= allowedDepth) return;

    const auto& pool =
        (P.useSinglePairTree && !isResTree)
        ? pairFeatPool()
        : (isResTree ? resFeatPool() : taskFeatPool());

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
    const auto& pool =
        (P.useSinglePairTree && !isResTree)
        ? pairFeatPool()
        : (isResTree ? resFeatPool() : taskFeatPool());

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
    case NodeKind::UNARY: {
        n.kind = NodeKind::FEATURE;
        n.left = n.right = -1;
        n.feat = pool[randInt(0, (int)pool.size() - 1)];
        break;
    }
    case NodeKind::BINARY: {
        static const BinaryOp ops[] = {
            BinaryOp::ADD,
            BinaryOp::SUB,
            BinaryOp::MUL,
            BinaryOp::DIV,
            BinaryOp::MIN,
            BinaryOp::MAX
        };
        n.bop = ops[randInt(0, 5)];
        break;
    }
    }
}

void TreeEA::mutateStruct(GPTree& t, bool isResTree) {
    if (t.nodes.empty()) return;
    int i = pickRandomNode(t);
    if (i < 0) return;

    const auto& pool =
        (P.useSinglePairTree && !isResTree)
        ? pairFeatPool()
        : (isResTree ? resFeatPool() : taskFeatPool());

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
    else {
        t.nodes.reserve(t.nodes.size() + 2);

        t.nodes[i].kind = NodeKind::BINARY;
        {
            static const BinaryOp ops[] = {
                BinaryOp::ADD,
                BinaryOp::SUB,
                BinaryOp::MUL,
                BinaryOp::DIV,
                BinaryOp::MIN,
                BinaryOp::MAX
            };
            t.nodes[i].bop = ops[randInt(0, 5)];
        }

        GPNode L{}, R{};
        if (rand01() < 0.5) { L.kind = NodeKind::FEATURE; L.feat = pool[randInt(0, (int)pool.size() - 1)]; }
        else { L.kind = NodeKind::CONST; L.constant = 0.0; }

        if (rand01() < 0.5) { R.kind = NodeKind::FEATURE; R.feat = pool[randInt(0, (int)pool.size() - 1)]; }
        else { R.kind = NodeKind::CONST; R.constant = 0.0; }

        L.left = L.right = -1;
        R.left = R.right = -1;

        int leftIdx = (int)t.nodes.size();  t.nodes.push_back(L);
        int rightIdx = (int)t.nodes.size(); t.nodes.push_back(R);

        t.nodes[i].left = leftIdx;
        t.nodes[i].right = rightIdx;
    }

    clampDepth(t, P.maxDepth, isResTree);
}

void TreeEA::mutateMacroSubtreeReplace(GPTree& t, bool isResTree) {
    if (t.isEmpty()) return;

    const int replaceIdx = pickRandomNode(t);
    if (replaceIdx < 0) return;

    const int minD = std::max(2, P.maxDepth / 2);
    const int maxD = std::max(2, P.maxDepth);
    const int regrowDepth = randInt(minD, maxD);

    GPTree donor = isResTree
        ? GPTree::RandomTreeRES(rng, regrowDepth)
        : (P.useSinglePairTree
            ? GPTree::RandomTreePAIR(rng, regrowDepth)
            : GPTree::RandomTreeMS(rng, regrowDepth));

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

double TreeEA::objNorm(const GP_Individual& x, int objId) const {
    return (objId == 0) ? x.msNorm : x.costNorm;
}


std::vector<std::pair<int, int>> TreeEA::selectParentsBNTGA(int populationSize)
{
    std::vector<std::pair<int, int>> selectedParents;

    if (archive_.size() < 2)
    {
        if (!archive_.empty()) selectedParents.emplace_back(0, 0);
        return selectedParents;
    }

    const int objectiveId = randInt(0, 1);

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

void TreeEA::run() {
    std::vector<GP_Individual> pop;
    initPopulation(pop);

    archive_.clear();
    archive_.reserve(P.popSize * 2);
    copyToArchiveWithFiltering(pop);

    auto logNodeSnapshot = [&](size_t generation, bool overwrite) {
        if (!P.logNodeDistribution) {
            return;
        }

        const bool useArchive = P.nodeStatsUseArchive;
        const auto& source = useArchive ? archive_ : pop;

        writeNodeDistributionSnapshot(
            source,
            generation,
            P.useSinglePairTree,
            useArchive ? "archive" : "population",
            overwrite);
        };

    logNodeSnapshot(0, true);

    for (size_t gen = 0; gen < P.generations; ++gen) {
        std::vector<GP_Individual> offspring;
        offspring.reserve(P.popSize);

        auto pairs = selectParentsBNTGA((int)P.popSize);
        const std::vector<GP_Individual>& archiveSnap = archive_;

        for (auto [ia, ib] : pairs) {
            if (ia < 0 || ib < 0 || ia >= (int)archiveSnap.size() || ib >= (int)archiveSnap.size()) {
                continue;
            }

            GP_Individual c1 = archiveSnap[ia];
            GP_Individual c2 = archiveSnap[ib];

            if (rand01() < P.pCrossover) {
                crossover(c1.taskTree, c2.taskTree, false);
            }
            if (!P.useSinglePairTree && rand01() < P.pCrossover) {
                crossover(c1.resTree, c2.resTree, true);
            }

            if (rand01() < P.pMutParam) {
                mutateParam(c1.taskTree, false);
            }
            if (rand01() < P.pMutParam) {
                mutateParam(c2.taskTree, false);
            }
            if (!P.useSinglePairTree && rand01() < P.pMutParam) {
                mutateParam(c1.resTree, true);
            }
            if (!P.useSinglePairTree && rand01() < P.pMutParam) {
                mutateParam(c2.resTree, true);
            }

            auto structOrMacro = [&](GPTree& tr, bool isRes) {
                if (rand01() < P.pMutMacroSubtree) {
                    mutateMacroSubtreeReplace(tr, isRes);
                }
                else if (rand01() < P.pMutStruct) {
                    mutateStruct(tr, isRes);
                }
                };

            structOrMacro(c1.taskTree, false);
            structOrMacro(c2.taskTree, false);

            if (!P.useSinglePairTree) {
                structOrMacro(c1.resTree, true);
                structOrMacro(c2.resTree, true);
            }

            auto e1 = evaluate(c1);
            offspring.push_back(std::move(e1));

            if (offspring.size() < P.popSize) {
                auto e2 = evaluate(c2);
                offspring.push_back(std::move(e2));
            }
        }

        if (offspring.empty()) {
            break;
        }

        copyToArchiveWithFiltering(offspring);
        pop.swap(offspring);

        const size_t currentGeneration = gen + 1;
        if (P.logNodeDistribution) {
            const bool hitStep =
                (P.nodeStatsEvery > 0) &&
                ((currentGeneration % P.nodeStatsEvery) == 0);

            const bool isLast =
                (currentGeneration == P.generations);

            if (hitStep || isLast) {
                logNodeSnapshot(currentGeneration, false);
            }
        }
    }
}


