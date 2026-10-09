#include "TreeEASO.hpp"
#include "../rules/GPTreeRule.hpp"
#include "../rules/GPTreeResRule.hpp"
#include "problem/problems/MSRCPSP/CScheduler.h"
#include "utils/logger/CExperimentLogger.h"
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
#include <fstream>
#include <filesystem>
#include <unordered_set>

namespace gphh_so {

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

    static void logPopulationStats(int generation, const std::vector<GP_Individual>& pop) {
        if (pop.empty()) return;

        auto itBest = std::min_element(pop.begin(), pop.end(),
            [](const GP_Individual& a, const GP_Individual& b) {
                return a.fitness < b.fitness;
            });

        auto itWorst = std::max_element(pop.begin(), pop.end(),
            [](const GP_Individual& a, const GP_Individual& b) {
                return a.fitness < b.fitness;
            });

        double sum = 0.0;
        for (const auto& ind : pop) {
            sum += ind.fitness;
        }
        const double avg = sum / static_cast<double>(pop.size());

        std::ostringstream oss;
        oss << generation << ';'
            << itBest->fitness << ';'
            << itWorst->fitness << ';'
            << avg;

        CExperimentLogger::AddLine(oss.str().c_str());
    }

    static void logPopulationDiversity(
        int generation,
        const std::vector<GP_Individual>& pop,
        bool useSinglePairTree,
        bool enabled)
    {
        if (!enabled || pop.empty()) return;

        const std::string filePath =
            CExperimentLogger::m_OutputDataPathPrefix + "/population_diversity.csv";

        const bool writeHeader = !std::filesystem::exists(filePath);

        std::unordered_set<std::uint64_t> uniqueIndividuals;
        std::unordered_set<std::uint64_t> uniqueTaskTrees;
        std::unordered_set<std::uint64_t> uniqueResTrees;

        uniqueIndividuals.reserve(pop.size() * 2);
        uniqueTaskTrees.reserve(pop.size() * 2);
        uniqueResTrees.reserve(pop.size() * 2);

        double avgTaskNodes = 0.0;
        double avgResNodes = 0.0;
        double avgFitness = 0.0;

        for (const auto& ind : pop) {
            uniqueIndividuals.insert(buildIndividualCacheHash(ind, useSinglePairTree));
            uniqueTaskTrees.insert(buildTreeCacheHash(ind.taskTree));

            avgTaskNodes += static_cast<double>(ind.taskTree.nodes.size());
            avgFitness += ind.fitness;

            if (!useSinglePairTree) {
                uniqueResTrees.insert(buildTreeCacheHash(ind.resTree));
                avgResNodes += static_cast<double>(ind.resTree.nodes.size());
            }
        }

        avgTaskNodes /= static_cast<double>(pop.size());
        avgFitness /= static_cast<double>(pop.size());

        if (!useSinglePairTree) {
            avgResNodes /= static_cast<double>(pop.size());
        }

        const std::size_t uniqueIndividualCount = uniqueIndividuals.size();
        const std::size_t uniqueTaskCount = uniqueTaskTrees.size();
        const std::size_t uniqueResCount = useSinglePairTree ? 0 : uniqueResTrees.size();

        const double duplicateRatio =
            1.0 - (static_cast<double>(uniqueIndividualCount) / static_cast<double>(pop.size()));

        std::ofstream out(filePath, std::ofstream::out | std::ofstream::app);
        if (!out.is_open()) {
            std::cerr << "Unable to open file: " << filePath << std::endl;
            return;
        }

        if (writeHeader) {
            out << "generation"
                << ";unique_individuals"
                << ";duplicate_ratio"
                << ";unique_task_trees"
                << ";unique_res_trees"
                << ";avg_task_nodes"
                << ";avg_res_nodes"
                << ";avg_fitness"
                << std::endl;
        }

        out << generation
            << ";" << uniqueIndividualCount
            << ";" << duplicateRatio
            << ";" << uniqueTaskCount
            << ";" << uniqueResCount
            << ";" << avgTaskNodes
            << ";" << avgResNodes
            << ";" << avgFitness
            << std::endl;
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

static int featureIndex(const std::vector<FeatureId>& pool, FeatureId f) {
    for (int i = 0; i < (int)pool.size(); ++i) {
        if (pool[i] == f) return i;
    }
    return -1;
}

static int weightedIndex(std::mt19937& rng, const std::vector<double>& weights) {
    if (weights.empty()) return -1;

    double total = 0.0;
    for (double w : weights) {
        if (w > 0.0 && std::isfinite(w)) {
            total += w;
        }
    }

    if (total <= 0.0) {
        std::uniform_int_distribution<int> U(0, (int)weights.size() - 1);
        return U(rng);
    }

    std::uniform_real_distribution<double> U(0.0, total);
    double r = U(rng);
    double acc = 0.0;

    for (int i = 0; i < (int)weights.size(); ++i) {
        const double w = (weights[i] > 0.0 && std::isfinite(weights[i])) ? weights[i] : 0.0;
        acc += w;
        if (r <= acc) return i;
    }

    return (int)weights.size() - 1;
}

TreeEASO::TreeEASO(Instance& I, const GPEA_Params& params, CScheduler* imopseScheduler, bool isTAProblem)
    : inst(I),
    workInst_(I),
    P(params),
    rng(static_cast<unsigned>(P.seed)),
    imopseSch_(imopseScheduler),
    imopseIsTA_(isTAProblem) {
    taskFeatureUniverse_ = taskFeatPool();
    resFeatureUniverse_ = resFeatPool();
    pairFeatureUniverse_ = pairFeatPool();

    taskFeatureScores_.assign(taskFeatureUniverse_.size(), 1.0);
    resFeatureScores_.assign(resFeatureUniverse_.size(), 1.0);
    pairFeatureScores_.assign(pairFeatureUniverse_.size(), 1.0);
    gp::buildCPM(inst, cpm);
    gp::setCPMPrecalc(&cpm);


    bounds = compute_imopse_bounds(inst);

    if (P.useImopseEvaluate && imopseSch_ && imopseIsTA_) {
        bounds.ms_min = (int)std::lround((double)imopseSch_->GetMinDuration());
        bounds.ms_max = (int)std::lround((double)imopseSch_->GetMaxDuration());
        bounds.cost_min = (double)imopseSch_->GetMinCost();
        bounds.cost_max = (double)imopseSch_->GetMaxCost();

        if (bounds.ms_max <= bounds.ms_min) {
            bounds.ms_max = bounds.ms_min + 1;
        }
        if (bounds.cost_max <= bounds.cost_min) {
            bounds.cost_max = bounds.cost_min + 1.0;
        }
    }

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
    }

    auto normVals = imopse_minmax_normalize(ms, cost, bounds);
    ind.msNorm = normVals.first;
    ind.costNorm = normVals.second;

    ind.makespan = ms;
    ind.cost = cost;


    const double fitnessNorm = P.weight * ind.costNorm + (1.0 - P.weight) * ind.msNorm;
    const double fitnessRaw = P.weight * ind.cost + (1.0 - P.weight) * (double)ind.makespan;

    ind.fitness = fitnessNorm;

    g_evalCache.emplace(cacheKey, EvalCacheEntry{
        ind.makespan,
        ind.cost,
        ind.msNorm,
        ind.costNorm,
        ind.fitness
        });

    return ind;
}

GP_Individual TreeEASO::createRandomIndividual() {
    GP_Individual ind;

    ind.taskTree = P.useSinglePairTree
        ? GPTree::RandomTreePAIR(rng, P.maxDepth)
        : GPTree::RandomTreeMS(rng, P.maxDepth);

    if (!P.useSinglePairTree) {
        ind.resTree = GPTree::RandomTreeRES(rng, P.maxDepth);
    }

    return evaluate(ind);
}

void TreeEASO::mutateCloneRecovery(GP_Individual& ind) {

    applyMutation(ind.taskTree, false);

    if (!P.useSinglePairTree) {
        applyMutation(ind.resTree, true);
    }
}

bool TreeEASO::insertWithCloneAvoidance(
    std::vector<GP_Individual>& target,
    GP_Individual candidate,
    std::unordered_set<std::uint64_t>& seenHashes)
{
    if (!P.avoidClonesInPopulation) {
        target.push_back(evaluate(candidate));
        return true;
    }

    const int maxCloneRetries = std::max(0, P.cloneMutationRetries);
    int cloneStreak = 0;

    while (cloneStreak < maxCloneRetries) {
        const std::uint64_t h = buildIndividualCacheHash(candidate, P.useSinglePairTree);

        if (seenHashes.insert(h).second) {
            target.push_back(evaluate(candidate));
            return true;
        }

        ++mutDbg_.cloneHits;
        ++cloneStreak;

        if (cloneStreak >= maxCloneRetries) {
            break;
        }

        ++mutDbg_.cloneMutationAttempts;
        mutateCloneRecovery(candidate);
    }

    ++mutDbg_.cloneRandomReplacements;

    constexpr int kRandomFallbackAttempts = 64;

    for (int i = 0; i < kRandomFallbackAttempts; ++i) {
        GP_Individual randomCandidate = createRandomIndividual();
        const std::uint64_t h = buildIndividualCacheHash(randomCandidate, P.useSinglePairTree);

        if (seenHashes.insert(h).second) {
            target.push_back(std::move(randomCandidate));
            return true;
        }

        ++mutDbg_.cloneHits;
    }

    GP_Individual randomCandidate = createRandomIndividual();
    seenHashes.insert(buildIndividualCacheHash(randomCandidate, P.useSinglePairTree));
    target.push_back(std::move(randomCandidate));

    return true;
}

void TreeEASO::initPopulation(std::vector<GP_Individual>& pop) {
    pop.clear();
    pop.reserve(P.popSize);

    std::unordered_set<std::uint64_t> seenHashes;

    if (P.avoidClonesInPopulation) {
        seenHashes.reserve(P.popSize * 2);
    }

    while (pop.size() < P.popSize) {
        GP_Individual ind = createRandomIndividual();
        insertWithCloneAvoidance(pop, std::move(ind), seenHashes);
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

int TreeEASO::nodeArity(const GPNode& n) const {
    switch (n.kind) {
    case NodeKind::CONST:
    case NodeKind::FEATURE:
        return 0;
    case NodeKind::UNARY:
        return 1;
    case NodeKind::BINARY:
        return 2;
    }
    return 0;
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

void TreeEASO::subtreeCrossover(GPTree& a, GPTree& b, bool isResTree) {
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


double TreeEASO::subtreeFeatureScore(const GPTree& t, int rootIndex, bool isResTree) const {
    if (!t.validIndex(rootIndex)) return 0.0;

    const auto& pool = mutationFeaturePool(isResTree);
    const auto& scores = mutationFeatureScores(isResTree);

    GPTree sub = t.extractSubtree(rootIndex);
    if (sub.isEmpty()) return 0.0;

    double totalScore = 0.0;
    for (const auto& n : sub.nodes) {
        if (n.kind != NodeKind::FEATURE) continue;

        const int fi = featureIndex(pool, n.feat);
        const double popularity =
            (fi >= 0 && fi < (int)scores.size()) ? scores[fi] : 1.0;

        totalScore += 1.0 / std::max(popularity, 1e-9);
    }

    return totalScore;
}

FeatureId TreeEASO::dominantSubtreeFeature(const GPTree& t, int rootIndex, bool isResTree) const {
    const auto& pool = mutationFeaturePool(isResTree);
    const auto& scores = mutationFeatureScores(isResTree);

    GPTree sub = t.extractSubtree(rootIndex);

    double bestScore = -1.0;
    FeatureId bestFeat = pool.empty() ? FeatureId{} : pool[0];

    for (const auto& n : sub.nodes) {
        if (n.kind != NodeKind::FEATURE) continue;

        const int fi = featureIndex(pool, n.feat);
        const double popularity =
            (fi >= 0 && fi < (int)scores.size()) ? scores[fi] : 1.0;

        const double score = 1.0 / std::max(popularity, 1e-9);
        if (score > bestScore) {
            bestScore = score;
            bestFeat = n.feat;
        }
    }

    return bestFeat;
}

bool TreeEASO::subtreeContainsFeature(const GPTree& t, int rootIndex, FeatureId feat) const {
    GPTree sub = t.extractSubtree(rootIndex);
    for (const auto& n : sub.nodes) {
        if (n.kind == NodeKind::FEATURE && n.feat == feat) {
            return true;
        }
    }
    return false;
}

void TreeEASO::featureAwareCrossover(GPTree& a, GPTree& b, bool isResTree) {
    if (a.isEmpty() || b.isEmpty()) return;

    GPTree A0 = a, B0 = b;

    std::vector<int> candidatesA;
    std::vector<double> weightsA;
    candidatesA.reserve(A0.nodes.size());
    weightsA.reserve(A0.nodes.size());

    for (int i = 0; i < (int)A0.nodes.size(); ++i) {
        const double s = subtreeFeatureScore(A0, i, isResTree);
        if (s <= 0.0) continue;

        candidatesA.push_back(i);

        const double depthPenalty = 1.0 + 0.15 * (double)A0.nodeDepth(i);
        weightsA.push_back(s / depthPenalty);
    }

    if (candidatesA.empty()) {
        subtreeCrossover(a, b, isResTree);
        return;
    }

    const int pickA = weightedIndex(rng, weightsA);
    const int ia = (pickA >= 0)
        ? candidatesA[pickA]
        : candidatesA[randInt(0, (int)candidatesA.size() - 1)];

        const int arityA = nodeArity(A0.nodes[ia]);
        const int depthA = A0.nodeDepth(ia);
        const FeatureId focusFeat = dominantSubtreeFeature(A0, ia, isResTree);

        std::vector<int> candidatesB;
        std::vector<double> weightsB;
        candidatesB.reserve(B0.nodes.size());
        weightsB.reserve(B0.nodes.size());

        for (int j = 0; j < (int)B0.nodes.size(); ++j) {
            if (nodeArity(B0.nodes[j]) != arityA) continue;

            const double s = subtreeFeatureScore(B0, j, isResTree);
            if (s <= 0.0) continue;

            double w = s;

            if (subtreeContainsFeature(B0, j, focusFeat)) {
                w *= 2.0;
            }

            const int depthDiff = std::abs(B0.nodeDepth(j) - depthA);
            if (depthDiff <= 1) {
                w *= 1.5;
            }

            candidatesB.push_back(j);
            weightsB.push_back(w);
        }

        if (candidatesB.empty()) {
            subtreeCrossover(a, b, isResTree);
            return;
        }

        const int pickB = weightedIndex(rng, weightsB);
        const int ib = (pickB >= 0)
            ? candidatesB[pickB]
            : candidatesB[randInt(0, (int)candidatesB.size() - 1)];

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

void TreeEASO::applyCrossover(GPTree& a, GPTree& b, bool isResTree) {
    const double subtreeShare = std::max(0.0, P.pSubtreeCrossover);
    const double featureAwareShare = std::max(0.0, P.pFeatureAwareCrossover);

    const double total = subtreeShare + featureAwareShare;

    if (total <= 0.0) {
        subtreeCrossover(a, b, isResTree);
        return;
    }

    const double r = rand01() * total;

    if (r < subtreeShare) {
        subtreeCrossover(a, b, isResTree);
    }
    else {
        featureAwareCrossover(a, b, isResTree);
    }
}

void TreeEASO::subtreeMutation(GPTree& t, bool isResTree) {
    if (t.isEmpty()) return;

    const int replaceIdx = pickRandomNode(t);
    if (replaceIdx < 0) return;

    const int maxDonorDepth = std::max(2, P.maxDepth - t.nodeDepth(replaceIdx));
    const int donorDepth = randInt(1, maxDonorDepth);

    GPTree donor = isResTree
        ? GPTree::RandomTreeRES(rng, donorDepth)
        : (P.useSinglePairTree
            ? GPTree::RandomTreePAIR(rng, donorDepth)
            : GPTree::RandomTreeMS(rng, donorDepth));

    clampDepth(donor, P.maxDepth, isResTree);

    GPTree base = t;
    GPTree child = base.graftedWith(replaceIdx, donor);
    clampDepth(child, P.maxDepth, isResTree);

    if (!child.isEmpty() && child.nodeCount() <= 100000 && child.isStructurallySound()) {
        t = std::move(child);
    }
}

bool TreeEASO::pointMutateNodeAt(GPTree& t, int idx, bool isResTree) {
    if (idx < 0 || idx >= (int)t.nodes.size()) return false;

    auto& n = t.nodes[idx];
    const auto& pool =
        (P.useSinglePairTree && !isResTree)
        ? pairFeatPool()
        : (isResTree ? resFeatPool() : taskFeatPool());

    switch (n.kind) {
    case NodeKind::CONST: {
        const double oldVal = n.constant;
        std::uniform_real_distribution<double> U(-5.0, 5.0);

        double newVal = oldVal;
        for (int tries = 0; tries < 8 && newVal == oldVal; ++tries) {
            newVal = U(rng);
        }
        n.constant = newVal;
        return (n.constant != oldVal);
    }

    case NodeKind::FEATURE: {
        if (pool.empty()) return false;

        const FeatureId oldFeat = n.feat;
        if (pool.size() == 1 && pool[0] == oldFeat) {
            return false;
        }

        FeatureId newFeat = oldFeat;
        for (int tries = 0; tries < 8 && newFeat == oldFeat; ++tries) {
            newFeat = pool[randInt(0, (int)pool.size() - 1)];
        }

        if (newFeat == oldFeat) return false;
        n.feat = newFeat;
        return true;
    }

    case NodeKind::UNARY: {
        const UnaryOp oldOp = n.uop;
        n.uop = (oldOp == UnaryOp::NEG) ? UnaryOp::ABS : UnaryOp::NEG;
        return (n.uop != oldOp);
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

        const BinaryOp oldOp = n.bop;
        BinaryOp newOp = oldOp;

        for (int tries = 0; tries < 8 && newOp == oldOp; ++tries) {
            newOp = ops[randInt(0, 5)];
        }

        if (newOp == oldOp) return false;
        n.bop = newOp;
        return true;
    }
    }

    return false;
}

void TreeEASO::pointMutation(GPTree& t, bool isResTree) {
    if (t.nodes.empty()) return;

    const int i = pickRandomNode(t);
    if (i < 0) return;

    (void)pointMutateNodeAt(t, i, isResTree);
}

void TreeEASO::pointMutationPerGene(GPTree& t, bool isResTree) {
    if (t.nodes.empty()) return;

    const std::uint64_t beforeHash = buildTreeCacheHash(t);
    mutDbg_.pointGeneCalls++;

    for (int i = 0; i < (int)t.nodes.size(); ++i) {
        mutDbg_.pointGeneVisitedNodes++;

        if (rand01() >= P.pMutation) {
            continue;
        }

        mutDbg_.pointGeneSelectedNodes++;

        const NodeKind kindBefore = t.nodes[i].kind;
        if (pointMutateNodeAt(t, i, isResTree)) {
            mutDbg_.pointGeneChangedNodes++;

            switch (kindBefore) {
            case NodeKind::CONST:
                mutDbg_.pointGeneChangedConst++;
                break;
            case NodeKind::FEATURE:
                mutDbg_.pointGeneChangedFeature++;
                break;
            case NodeKind::UNARY:
                mutDbg_.pointGeneChangedUnary++;
                break;
            case NodeKind::BINARY:
                mutDbg_.pointGeneChangedBinary++;
                break;
            }
        }
    }

    const std::uint64_t afterHash = buildTreeCacheHash(t);
    if (afterHash != beforeHash) {
        mutDbg_.pointGeneChangedTrees++;
    }
}

void TreeEASO::hoistMutation(GPTree& t, bool isResTree) {
    if (t.isEmpty()) return;

    const int replaceIdx = pickRandomNode(t);
    if (replaceIdx < 0) return;

    GPTree selected = t.extractSubtree(replaceIdx);
    if (selected.isEmpty() || selected.nodeCount() <= 1) return;

    int hoistIdx = pickRandomNode(selected);
    if (hoistIdx < 0) return;

    GPTree donor = selected.extractSubtree(hoistIdx);
    clampDepth(donor, P.maxDepth, isResTree);

    GPTree base = t;
    GPTree child = base.graftedWith(replaceIdx, donor);
    clampDepth(child, P.maxDepth, isResTree);

    if (!child.isEmpty() && child.nodeCount() <= 100000 && child.isStructurallySound()) {
        t = std::move(child);
    }
}

void TreeEASO::accumulateFeatureCounts(const GPTree& t,
    const std::vector<FeatureId>& universe,
    std::vector<double>& counts) const
{
    if (counts.size() != universe.size()) {
        counts.assign(universe.size(), 0.0);
    }

    for (const auto& n : t.nodes) {
        if (n.kind != NodeKind::FEATURE) continue;

        const int fi = featureIndex(universe, n.feat);
        if (fi >= 0) {
            counts[fi] += 1.0;
        }
    }
}

const std::vector<FeatureId>& TreeEASO::mutationFeaturePool(bool isResTree) const {
    if (P.useSinglePairTree && !isResTree) {
        return pairFeatureUniverse_;
    }
    return isResTree ? resFeatureUniverse_ : taskFeatureUniverse_;
}

const std::vector<double>& TreeEASO::mutationFeatureScores(bool isResTree) const {
    if (P.useSinglePairTree && !isResTree) {
        return pairFeatureScores_;
    }
    return isResTree ? resFeatureScores_ : taskFeatureScores_;
}

void TreeEASO::refreshAdaptiveFeatureStats(const std::vector<GP_Individual>& pop) {
    if (pop.empty()) return;

    constexpr double kEliteFrac = 0.20;
    constexpr double kDecay = 0.70;
    constexpr double kPrior = 1.0;

    const size_t eliteByFrac = std::max<size_t>(1, (size_t)std::ceil((double)pop.size() * kEliteFrac));
    const size_t eliteN = std::min(pop.size(), std::max(P.eliteCount, eliteByFrac));

    std::vector<size_t> order(pop.size());
    std::iota(order.begin(), order.end(), 0);
    std::partial_sort(order.begin(), order.begin() + eliteN, order.end(),
        [&](size_t a, size_t b) { return pop[a].fitness < pop[b].fitness; });

    std::vector<double> taskCounts(taskFeatureUniverse_.size(), 0.0);
    std::vector<double> resCounts(resFeatureUniverse_.size(), 0.0);
    std::vector<double> pairCounts(pairFeatureUniverse_.size(), 0.0);

    for (size_t rank = 0; rank < eliteN; ++rank) {
        const auto& ind = pop[order[rank]];
        if (P.useSinglePairTree) {
            accumulateFeatureCounts(ind.taskTree, pairFeatureUniverse_, pairCounts);
        }
        else {
            accumulateFeatureCounts(ind.taskTree, taskFeatureUniverse_, taskCounts);
            accumulateFeatureCounts(ind.resTree, resFeatureUniverse_, resCounts);
        }
    }

    auto blendScores = [&](const std::vector<double>& counts,
        std::vector<double>& scores,
        size_t universeSize) {
            if (scores.size() != universeSize) {
                scores.assign(universeSize, 1.0 / std::max<size_t>(1, universeSize));
            }

            double total = 0.0;
            for (double c : counts) total += c;
            const double denom = total + kPrior * (double)universeSize;

            for (size_t i = 0; i < universeSize; ++i) {
                const double observed = (counts[i] + kPrior) / std::max(denom, 1e-12);
                if (!adaptiveStatsReady_) {
                    scores[i] = observed;
                }
                else {
                    scores[i] = kDecay * scores[i] + (1.0 - kDecay) * observed;
                }
            }
        };

    if (P.useSinglePairTree) {
        blendScores(pairCounts, pairFeatureScores_, pairFeatureUniverse_.size());
    }
    else {
        blendScores(taskCounts, taskFeatureScores_, taskFeatureUniverse_.size());
        blendScores(resCounts, resFeatureScores_, resFeatureUniverse_.size());
    }

    adaptiveStatsReady_ = true;
}

int TreeEASO::pickGuidedFeatureNode(const GPTree& t, bool isResTree) {
    const auto& pool = mutationFeaturePool(isResTree);
    const auto& scores = mutationFeatureScores(isResTree);

    std::vector<int> featureNodes;
    std::vector<double> weights;

    featureNodes.reserve(t.nodes.size());
    weights.reserve(t.nodes.size());

    for (int i = 0; i < (int)t.nodes.size(); ++i) {
        if (t.nodes[i].kind != NodeKind::FEATURE) continue;

        featureNodes.push_back(i);

        const int fi = featureIndex(pool, t.nodes[i].feat);
        const double popularity = (fi >= 0 && fi < (int)scores.size()) ? scores[fi] : 1.0;
        weights.push_back(1.0 / std::max(popularity, 1e-9));
    }

    if (featureNodes.empty()) return -1;

    const int selected = weightedIndex(rng, weights);
    if (selected < 0) return featureNodes[randInt(0, (int)featureNodes.size() - 1)];
    return featureNodes[selected];
}

FeatureId TreeEASO::pickReplacementFeature(FeatureId current, bool isResTree) {
    const auto& pool = mutationFeaturePool(isResTree);
    const auto& scores = mutationFeatureScores(isResTree);

    std::vector<FeatureId> candidates;
    std::vector<double> weights;
    candidates.reserve(pool.size());
    weights.reserve(pool.size());

    const double meanScore = scores.empty()
        ? 1.0
        : std::accumulate(scores.begin(), scores.end(), 0.0) / (double)scores.size();
    const double explorationFloor = 0.15 * std::max(meanScore, 1e-9);

    for (int i = 0; i < (int)pool.size(); ++i) {
        if (pool[i] == current) continue;
        candidates.push_back(pool[i]);
        const double popularity = (i < (int)scores.size()) ? scores[i] : 1.0;
        weights.push_back(popularity + explorationFloor);
    }

    if (candidates.empty()) {
        return current;
    }

    const int selected = weightedIndex(rng, weights);
    if (selected < 0) {
        return candidates[randInt(0, (int)candidates.size() - 1)];
    }
    return candidates[selected];
}

bool TreeEASO::featureGuidedMutateNodeAt(GPTree& t, int idx, bool isResTree) {
    if (idx < 0 || idx >= (int)t.nodes.size()) return false;

    auto& n = t.nodes[idx];
    if (n.kind != NodeKind::FEATURE) return false;

    const FeatureId replacement = pickReplacementFeature(n.feat, isResTree);
    if (replacement == n.feat) return false;

    n.feat = replacement;
    return true;
}

void TreeEASO::featureGuidedPointMutation(GPTree& t, bool isResTree) {
    if (t.nodes.empty()) return;

    const std::uint64_t beforeHash = buildTreeCacheHash(t);
    mutDbg_.guidedCalls++;

    const int selectedIdx = pickGuidedFeatureNode(t, isResTree);
    if (selectedIdx < 0) {
        mutDbg_.guidedFallbackToPoint++;
        pointMutation(t, isResTree);

        const std::uint64_t afterHash = buildTreeCacheHash(t);
        if (afterHash != beforeHash) {
            mutDbg_.guidedChangedTrees++;
        }
        return;
    }

    auto& n = t.nodes[selectedIdx];
    if (n.kind != NodeKind::FEATURE) {
        mutDbg_.guidedFallbackToPoint++;
        pointMutation(t, isResTree);

        const std::uint64_t afterHash = buildTreeCacheHash(t);
        if (afterHash != beforeHash) {
            mutDbg_.guidedChangedTrees++;
        }
        return;
    }

    const FeatureId replacement = pickReplacementFeature(n.feat, isResTree);
    if (replacement == n.feat) {
        mutDbg_.guidedFallbackToPoint++;
        pointMutation(t, isResTree);

        const std::uint64_t afterHash = buildTreeCacheHash(t);
        if (afterHash != beforeHash) {
            mutDbg_.guidedChangedTrees++;
        }
        return;
    }

    n.feat = replacement;

    const std::uint64_t afterHash = buildTreeCacheHash(t);
    if (afterHash != beforeHash) {
        mutDbg_.guidedChangedTrees++;
    }
}

void TreeEASO::featureGuidedPointMutationPerGene(GPTree& t, bool isResTree) {
    if (t.nodes.empty()) return;

    const std::uint64_t beforeHash = buildTreeCacheHash(t);
    mutDbg_.geneLevelCalls++;

    int mutatedCount = 0;

    for (int i = 0; i < (int)t.nodes.size(); ++i) {
        auto& n = t.nodes[i];
        if (n.kind != NodeKind::FEATURE) continue;

        mutDbg_.featureNodesVisited++;

        if (rand01() < P.pMutation) {
            mutDbg_.featureNodesSelected++;

            const FeatureId replacement = pickReplacementFeature(n.feat, isResTree);
            if (replacement != n.feat) {
                n.feat = replacement;
                ++mutatedCount;
                mutDbg_.featureNodesChanged++;
            }
        }
    }

    const std::uint64_t afterHash = buildTreeCacheHash(t);
    if (afterHash != beforeHash) {
        mutDbg_.geneLevelChangedTrees++;
    }
}

void TreeEASO::geneLevelMutationRoulette(GPTree& t, bool isResTree) {
    if (t.nodes.empty()) return;

    const std::uint64_t beforeHash = buildTreeCacheHash(t);
    mutDbg_.rouletteGeneCalls++;

    const double wPoint = std::max(0.0, P.pPointMutation);
    const double wGuided = std::max(0.0, P.pFeatureGuidedPointMutation);

    if (wPoint + wGuided <= 0.0) return;

    for (int i = 0; i < (int)t.nodes.size(); ++i) {
        mutDbg_.rouletteVisitedNodes++;

        if (rand01() >= P.pMutation) {
            continue;
        }

        mutDbg_.rouletteSelectedNodes++;

        const bool isFeatureNode = (t.nodes[i].kind == NodeKind::FEATURE);

        const double activePoint = wPoint;
        const double activeGuided = isFeatureNode ? wGuided : 0.0;
        const double sum = activePoint + activeGuided;

        if (sum <= 0.0) {
            continue;
        }

        const double r = rand01() * sum;
        bool changed = false;

        if (r < activePoint) {
            mutDbg_.roulettePointAttempts++;
            changed = pointMutateNodeAt(t, i, isResTree);
            if (changed) {
                mutDbg_.roulettePointChanged++;
                mutDbg_.rouletteChangedNodes++;
            }
        }
        else {
            mutDbg_.rouletteGuidedAttempts++;
            changed = featureGuidedMutateNodeAt(t, i, isResTree);
            if (changed) {
                mutDbg_.rouletteGuidedChanged++;
                mutDbg_.rouletteChangedNodes++;
            }
        }
    }

    const std::uint64_t afterHash = buildTreeCacheHash(t);
    if (afterHash != beforeHash) {
        mutDbg_.rouletteGeneChangedTrees++;
    }
}

void TreeEASO::applyMutation(GPTree& t, bool isResTree) {
    const double total =
        std::max(0.0, P.pSubtreeMutation) +
        std::max(0.0, P.pPointMutation) +
        std::max(0.0, P.pHoistMutation) +
        std::max(0.0, P.pFeatureGuidedPointMutation);

    if (total <= 0.0) {
        subtreeMutation(t, isResTree);
        return;
    }

    const double r = rand01() * total;
    const double m1 = std::max(0.0, P.pSubtreeMutation);
    const double m2 = m1 + std::max(0.0, P.pPointMutation);
    const double m3 = m2 + std::max(0.0, P.pHoistMutation);

    if (r < m1) {
        subtreeMutation(t, isResTree);
    }
    else if (r < m2) {
        pointMutation(t, isResTree);
    }
    else if (r < m3) {
        hoistMutation(t, isResTree);
    }
    else {
        featureGuidedPointMutation(t, isResTree);
    }
}

GP_Individual TreeEASO::run() {
    std::vector<GP_Individual> pop;
    initPopulation(pop);

    auto itBest0 = std::min_element(pop.begin(), pop.end(),
        [](const GP_Individual& a, const GP_Individual& b) { return a.fitness < b.fitness; });

    GP_Individual bestSoFar = *itBest0;
    logPopulationStats(0, pop);
    logPopulationDiversity(0, pop, P.useSinglePairTree, P.logPopulationDiversity);

    for (size_t gen = 0; gen < P.generations; ++gen) {
        mutDbg_.reset();
        refreshAdaptiveFeatureStats(pop);
        std::vector<GP_Individual> offspring;
        offspring.reserve(P.popSize);

        while (offspring.size() < P.popSize) {
            const GP_Individual& p1 = tournament(pop, P.tournamentK);
            const GP_Individual& p2 = tournament(pop, P.tournamentK);

            GP_Individual c1 = p1;
            GP_Individual c2 = p2;

            if (rand01() < P.pCrossover) {
                applyCrossover(c1.taskTree, c2.taskTree, false);
            }
            if (!P.useSinglePairTree && rand01() < P.pCrossover) {
                applyCrossover(c1.resTree, c2.resTree, true);
            }

            if (P.useGeneLevelMutation) {
                geneLevelMutationRoulette(c1.taskTree, false);
                geneLevelMutationRoulette(c2.taskTree, false);

                if (!P.useSinglePairTree) {
                    geneLevelMutationRoulette(c1.resTree, true);
                    geneLevelMutationRoulette(c2.resTree, true);
                }

                if (P.pMacroSubtreeMutation > 0.0 && rand01() < P.pMacroSubtreeMutation) {
                    const std::uint64_t beforeHash = buildTreeCacheHash(c1.taskTree);
                    mutDbg_.macroSubtreeCalls++;
                    subtreeMutation(c1.taskTree, false);
                    if (buildTreeCacheHash(c1.taskTree) != beforeHash) {
                        mutDbg_.macroSubtreeChangedTrees++;
                    }
                }

                if (P.pMacroSubtreeMutation > 0.0 && rand01() < P.pMacroSubtreeMutation) {
                    const std::uint64_t beforeHash = buildTreeCacheHash(c2.taskTree);
                    mutDbg_.macroSubtreeCalls++;
                    subtreeMutation(c2.taskTree, false);
                    if (buildTreeCacheHash(c2.taskTree) != beforeHash) {
                        mutDbg_.macroSubtreeChangedTrees++;
                    }
                }

                if (!P.useSinglePairTree) {
                    if (P.pMacroSubtreeMutation > 0.0 && rand01() < P.pMacroSubtreeMutation) {
                        const std::uint64_t beforeHash = buildTreeCacheHash(c1.resTree);
                        mutDbg_.macroSubtreeCalls++;
                        subtreeMutation(c1.resTree, true);
                        if (buildTreeCacheHash(c1.resTree) != beforeHash) {
                            mutDbg_.macroSubtreeChangedTrees++;
                        }
                    }

                    if (P.pMacroSubtreeMutation > 0.0 && rand01() < P.pMacroSubtreeMutation) {
                        const std::uint64_t beforeHash = buildTreeCacheHash(c2.resTree);
                        mutDbg_.macroSubtreeCalls++;
                        subtreeMutation(c2.resTree, true);
                        if (buildTreeCacheHash(c2.resTree) != beforeHash) {
                            mutDbg_.macroSubtreeChangedTrees++;
                        }
                    }
                }
            }
            else {
                if (rand01() < P.pMutation) {
                    applyMutation(c1.taskTree, false);
                }
                if (rand01() < P.pMutation) {
                    applyMutation(c2.taskTree, false);
                }
                if (!P.useSinglePairTree && rand01() < P.pMutation) {
                    applyMutation(c1.resTree, true);
                }
                if (!P.useSinglePairTree && rand01() < P.pMutation) {
                    applyMutation(c2.resTree, true);
                }
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

        std::unordered_set<std::uint64_t> nextHashes;

        if (P.avoidClonesInPopulation) {
            nextHashes.reserve(pop.size() * 2);
        }

        size_t E = std::min<size_t>(P.eliteCount, pop.size());

        if (E > 0) {
            std::vector<GP_Individual> tmp = pop;

            std::partial_sort(tmp.begin(), tmp.begin() + E, tmp.end(),
                [](const GP_Individual& a, const GP_Individual& b) {
                    return a.fitness < b.fitness;
                });

            for (size_t e = 0; e < E && next.size() < pop.size(); ++e) {
                insertWithCloneAvoidance(next, tmp[e], nextHashes);
            }
        }

        for (size_t i = 0; i < offspring.size() && next.size() < pop.size(); ++i) {
            insertWithCloneAvoidance(next, offspring[i], nextHashes);
        }

        while (next.size() < pop.size()) {
            insertWithCloneAvoidance(next, createRandomIndividual(), nextHashes);
        }

        pop.swap(next);
        logPopulationStats(static_cast<int>(gen) + 1, pop);
        logPopulationDiversity(static_cast<int>(gen) + 1, pop, P.useSinglePairTree, P.logPopulationDiversity);

        if (P.logMutationDebug && ((gen % 25) == 0 || gen + 1 == P.generations)) {
            std::cout
                << "[MUTDBG] gen=" << (gen + 1)
                << " point=" << mutDbg_.pointChangedTrees << "/" << mutDbg_.pointCalls
                << " guided=" << mutDbg_.guidedChangedTrees << "/" << mutDbg_.guidedCalls
                << " guidedFallback=" << mutDbg_.guidedFallbackToPoint
                << " geneTrees=" << mutDbg_.geneLevelChangedTrees << "/" << mutDbg_.geneLevelCalls
                << " featVisited=" << mutDbg_.featureNodesVisited
                << " featSelected=" << mutDbg_.featureNodesSelected
                << " featChanged=" << mutDbg_.featureNodesChanged
                << " pointGeneTrees=" << mutDbg_.pointGeneChangedTrees << "/" << mutDbg_.pointGeneCalls
                << " pointGeneVisited=" << mutDbg_.pointGeneVisitedNodes
                << " pointGeneSelected=" << mutDbg_.pointGeneSelectedNodes
                << " pointGeneChanged=" << mutDbg_.pointGeneChangedNodes
                << " pointKinds=("
                << "C:" << mutDbg_.pointGeneChangedConst << ","
                << "F:" << mutDbg_.pointGeneChangedFeature << ","
                << "U:" << mutDbg_.pointGeneChangedUnary << ","
                << "B:" << mutDbg_.pointGeneChangedBinary << ")"
                << " rouletteTrees=" << mutDbg_.rouletteGeneChangedTrees << "/" << mutDbg_.rouletteGeneCalls
                << " rouletteVisited=" << mutDbg_.rouletteVisitedNodes
                << " rouletteSelected=" << mutDbg_.rouletteSelectedNodes
                << " rouletteChanged=" << mutDbg_.rouletteChangedNodes
                << " roulettePoint=" << mutDbg_.roulettePointChanged << "/" << mutDbg_.roulettePointAttempts
                << " rouletteGuided=" << mutDbg_.rouletteGuidedChanged << "/" << mutDbg_.rouletteGuidedAttempts
                << " macroSubtree=" << mutDbg_.macroSubtreeChangedTrees << "/" << mutDbg_.macroSubtreeCalls
                << " cloneHits=" << mutDbg_.cloneHits
                << " cloneMut=" << mutDbg_.cloneMutationAttempts
                << " cloneRand=" << mutDbg_.cloneRandomReplacements
                << std::endl;
        }

        auto itBest = std::min_element(pop.begin(), pop.end(),
            [](const GP_Individual& a, const GP_Individual& b) { return a.fitness < b.fitness; });

        if (itBest->fitness < bestSoFar.fitness) {
            bestSoFar = *itBest;
        }
    }

    return bestSoFar;
}

}