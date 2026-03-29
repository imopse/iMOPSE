#include "TreeEASO.hpp"
#include "../rules/GPTreeRule.hpp"
#include "../rules/GPTreeResRule.hpp"
#include "problem/problems/MSRCPSP/CScheduler.h"
#include <cmath>
#include <cassert>
#include <algorithm>
#include <numeric>
#include <unordered_map>
#include <limits>
#include <sstream>
#include <iostream>
#include <string>
#include <cstdint>
#include <cstring>

namespace gpbntga_so {

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

TreeEASO::TreeEASO(Instance& I, const GPEA_Params& params, CScheduler* imopseScheduler, bool isTAProblem)
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


TreeEASO::~TreeEASO() {
    gp::setCPMPrecalc(nullptr);
}


double TreeEASO::rand01() {
    std::uniform_real_distribution<double> U(0.0, 1.0);
    return U(rng);
}
int TreeEASO::randInt(int lo, int hi) {
    std::uniform_int_distribution<int> U(lo, hi);
    return U(rng);
}

Instance& TreeEASO::resetWorkingInstance(bool clearAssignedResources) const {
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

GP_Individual TreeEASO::evaluate(const GP_Individual& src) const {
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

    const bool leanDecode = (P.useImopseEvaluate && imopseSch_ && imopseIsTA_);

    ScheduleOptions schedOpt;
    schedOpt.computeObjectiveStats = !leanDecode;
    schedOpt.keepTaskAssignedResources = !leanDecode;
    schedOpt.captureAssignedResByImopse = leanDecode;

    Instance& Ic = resetWorkingInstance(schedOpt.keepTaskAssignedResources);

    auto sim = Scheduler::withResources(Ic, taskRule, resRule, schedOpt, pairTree);

    int ms = sim.makespan;
    double cost = sim.totalCost;

    if (P.useImopseEvaluate && imopseSch_ && imopseIsTA_) {
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

    const double fitnessNorm = P.weight * ind.msNorm + (1.0 - P.weight) * ind.costNorm;
    const double fitnessRaw = P.weight * (double)ind.makespan + (1.0 - P.weight) * ind.cost;
    ind.fitness = P.useNormalization ? fitnessNorm : fitnessRaw;

    g_evalCache.emplace(cacheKey, EvalCacheEntry{
        ind.makespan,
        ind.cost,
        ind.msNorm,
        ind.costNorm,
        ind.fitness
        });

    return ind;
}

void TreeEASO::initPopulation(std::vector<GP_Individual>& pop) {
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


const GP_Individual& TreeEASO::tournament(const std::vector<GP_Individual>& pop, int k) {
    int best = -1;
    for (int i = 0; i < k; ++i) {
        int j = randInt(0, (int)pop.size() - 1);
        if (best == -1 || pop[j].fitness < pop[best].fitness) best = j;
    }
    return pop[best];
}

int TreeEASO::pickRandomNode(const GPTree& t) {
    if (t.nodes.empty()) return -1;
    return randInt(0, (int)t.nodes.size() - 1);
}

void TreeEASO::clampDepth(GPTree& t, int maxDepth, bool isResTree) {
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

void TreeEASO::crossover(GPTree& a, GPTree& b, bool isResTree) {
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


void TreeEASO::mutateParam(GPTree& t, bool isResTree) {
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

void TreeEASO::mutateStruct(GPTree& t, bool isResTree) {
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

void TreeEASO::mutateMacroSubtreeReplace(GPTree& t, bool isResTree) {
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

GP_Individual TreeEASO::run() {
    std::vector<GP_Individual> pop;
    initPopulation(pop);

    auto itBest0 = std::min_element(pop.begin(), pop.end(),
        [](const GP_Individual& a, const GP_Individual& b) { return a.fitness < b.fitness; });

    GP_Individual bestSoFar = *itBest0;

    for (size_t gen = 0; gen < P.generations; ++gen) {
        std::vector<GP_Individual> offspring;
        offspring.reserve(P.popSize);

        while (offspring.size() < P.popSize) {
            const GP_Individual& p1 = tournament(pop, P.tournamentK);
            const GP_Individual& p2 = tournament(pop, P.tournamentK);

            GP_Individual c1 = p1;
            GP_Individual c2 = p2;

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
            offspring.push_back(e1);

            if (offspring.size() < P.popSize) {
                auto e2 = evaluate(c2);
                offspring.push_back(e2);
            }
        }

        std::vector<GP_Individual> next;
        next.reserve(pop.size());

        size_t E = std::min<size_t>(P.eliteCount, pop.size());
        if (E > 0) {
            std::vector<GP_Individual> tmp = pop;
            std::partial_sort(tmp.begin(), tmp.begin() + E, tmp.end(),
                [](const GP_Individual& a, const GP_Individual& b) { return a.fitness < b.fitness; });
            for (size_t e = 0; e < E; ++e) {
                next.push_back(tmp[e]);
            }
        }

        for (size_t i = 0; i < offspring.size() && next.size() < pop.size(); ++i) {
            next.push_back(offspring[i]);
        }

        pop.swap(next);

        auto itBest = std::min_element(pop.begin(), pop.end(),
            [](const GP_Individual& a, const GP_Individual& b) { return a.fitness < b.fitness; });

        if (itBest->fitness < bestSoFar.fitness) {
            bestSoFar = *itBest;
        }
    }

    return bestSoFar;
}

} // namespace gpbntga_so