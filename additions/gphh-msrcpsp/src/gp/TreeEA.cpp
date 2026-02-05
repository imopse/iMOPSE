#include "TreeEA.hpp"
#include "rules/GPTreeRule.hpp"
#include "rules/GPTreeResRule.hpp"
#include <cmath>
#include <cassert>
#include <algorithm>
#include <numeric>

static const std::vector<FeatureId>& taskFeatPool() {
    static const std::vector<FeatureId> v = GPTree::allFeatures();
    return v;
}
static const std::vector<FeatureId>& resFeatPool() {
    static const std::vector<FeatureId> v = {
        FeatureId::RES_WAGE,
        FeatureId::RES_SKILL_LEVEL,
        FeatureId::RES_FREE_TIME,
        FeatureId::RES_MULTI_SKILL,
        FeatureId::RES_UTILIZATION
    };
    return v;
}

TreeEA::TreeEA(Instance& I, const GPEA_Params& params)
    : inst(I), P(params), rng(static_cast<unsigned>(P.seed)) {
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


GP_Individual TreeEA::evaluate(const GP_Individual& src) const {
    GP_Individual ind = src;
    Instance Ic = inst;
    GPTreeRule    ruleT(ind.taskTree);
    GPTreeResRule ruleR(ind.resTree);

    auto res = Scheduler::withResources(Ic, ruleT, &ruleR);

    ind.makespan = res.makespan;
    ind.cost = res.totalCost;

    auto normVals = imopse_minmax_normalize(ind.makespan, ind.cost, bounds);
    ind.msNorm = normVals.first;
    ind.costNorm = normVals.second;

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
    if (t.root < 0 || t.root >= (int)t.nodes.size()) return;

    const auto& pool = isResTree ? resFeatPool() : taskFeatPool();

    while (depthOf(t, t.root) > maxDepth) {
        GPNode leaf{};
        if (rand01() < 0.7) {
            leaf.kind = NodeKind::FEATURE;
            leaf.feat = pool[randInt(0, (int)pool.size() - 1)];
        }
        else {
            leaf.kind = NodeKind::CONST;
            leaf.constant = 0.0;
        }
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
    case NodeKind::BINARY:
        n.bop = static_cast<BinaryOp>(randInt(0, 5));
        break;
    }
}

void TreeEA::mutateStruct(GPTree& t, bool isResTree) {
    if (t.nodes.empty()) return;
    int i = pickRandomNode(t);
    if (i < 0) return;

    const auto& pool = isResTree ? resFeatPool() : taskFeatPool();

    double r = rand01();
    if (r < 0.34) {
        auto& n = t.nodes[i];
        n.kind = NodeKind::FEATURE;
        n.left = n.right = -1;
        n.feat = pool[randInt(0, (int)pool.size() - 1)];
    }
    else if (r < 0.67) {
        auto& n = t.nodes[i];
        n.kind = NodeKind::CONST;
        n.left = n.right = -1;
        std::uniform_real_distribution<double> U(-5.0, 5.0);
        n.constant = U(rng);
    }
    else {
        t.nodes.reserve(t.nodes.size() + 2);

        t.nodes[i].kind = NodeKind::BINARY;
        t.nodes[i].bop = static_cast<BinaryOp>(randInt(0, 5));

        GPNode L{}, R{};
        if (rand01() < 0.5) { L.kind = NodeKind::FEATURE; L.feat = pool[randInt(0, (int)pool.size() - 1)]; }
        else { L.kind = NodeKind::CONST;   L.constant = 0.0; }

        if (rand01() < 0.5) { R.kind = NodeKind::FEATURE; R.feat = pool[randInt(0, (int)pool.size() - 1)]; }
        else { R.kind = NodeKind::CONST;   R.constant = 0.0; }

        int leftIdx = (int)t.nodes.size();
        t.nodes.push_back(L);
        int rightIdx = (int)t.nodes.size();
        t.nodes.push_back(R);

        t.nodes[i].left = leftIdx;
        t.nodes[i].right = rightIdx;
    }

    clampDepth(t, P.maxDepth, isResTree);
}


GP_Individual TreeEA::run() {
    std::vector<GP_Individual> pop;
    initPopulation(pop);

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

    GP_Individual bestSoFar = bestGen0_;


    for (size_t gen = 0; gen < P.generations; ++gen) {
        std::vector<GP_Individual> next;
        next.reserve(pop.size());

        {
            size_t E = std::min<size_t>(P.eliteCount, pop.size());
            std::vector<GP_Individual> tmp = pop;
            std::partial_sort(tmp.begin(), tmp.begin() + E, tmp.end(),
                [](const GP_Individual& a, const GP_Individual& b) {
                    return a.fitness < b.fitness;
                });
            for (size_t e = 0; e < E; ++e) next.push_back(tmp[e]);
        }

        while (next.size() < pop.size()) {
            const GP_Individual& p1 = tournament(pop, P.tournamentK);
            const GP_Individual& p2 = tournament(pop, P.tournamentK);

            GP_Individual c1 = p1;
            GP_Individual c2 = p2;

            if (rand01() < P.pCrossover) crossover(c1.taskTree, c2.taskTree, false);
            if (rand01() < P.pCrossover) crossover(c1.resTree, c2.resTree, true);

            if (rand01() < P.pMutParam)  mutateParam(c1.taskTree, false);
            if (rand01() < P.pMutParam)  mutateParam(c2.taskTree, false);
            if (rand01() < P.pMutParam)  mutateParam(c1.resTree, true);
            if (rand01() < P.pMutParam)  mutateParam(c2.resTree, true);

            if (rand01() < P.pMutStruct) mutateStruct(c1.taskTree, false);
            if (rand01() < P.pMutStruct) mutateStruct(c2.taskTree, false);
            if (rand01() < P.pMutStruct) mutateStruct(c1.resTree, true);
            if (rand01() < P.pMutStruct) mutateStruct(c2.resTree, true);

            next.push_back(evaluate(c1));
            if (next.size() < pop.size())
                next.push_back(evaluate(c2));
        }

        pop.swap(next);

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
    }

    return bestSoFar;
}
