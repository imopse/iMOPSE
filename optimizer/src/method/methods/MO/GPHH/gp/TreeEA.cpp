#include "TreeEA.hpp"
#include "../rules/GPTreeRule.hpp"
#include "../rules/GPTreeResRule.hpp"
#include <cmath>
#include <cassert>
#include <algorithm>
#include <numeric>

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

std::vector<std::vector<int>> TreeEA::nonDominatedSort(std::vector<GP_Individual>& pop) const
{
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

            if (dominatesMO(pop[p], pop[q])) {
                S[p].push_back(q);
            }
            else if (dominatesMO(pop[q], pop[p])) {
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

        pop[idx.front()].crowding = std::numeric_limits<double>::infinity();
        pop[idx.back()].crowding = std::numeric_limits<double>::infinity();

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
    int best = -1;

    auto better = [&](int a, int b) {
        if (pop[a].rank != pop[b].rank) return pop[a].rank < pop[b].rank;
        if (pop[a].crowding != pop[b].crowding) return pop[a].crowding > pop[b].crowding;
        return pop[a].fitness < pop[b].fitness;
        };

    for (int i = 0; i < k; ++i) {
        int j = randInt(0, (int)pop.size() - 1);
        if (best == -1 || better(j, best)) best = j;
    }
    return pop[best];
}

std::vector<GP_Individual> TreeEA::selectNextPopulationNSGA2(std::vector<GP_Individual>& combined)
{
    auto fronts = nonDominatedSort(combined);

    for (const auto& f : fronts) calcCrowdingDistance(combined, f);

    std::vector<GP_Individual> next;
    next.reserve(P.popSize);

    for (const auto& f : fronts) {
        if (f.empty()) break;

        if (next.size() + f.size() <= P.popSize) {
            for (int idx : f) next.push_back(combined[idx]);
        }
        else {
            std::vector<int> tmp = f;
            std::sort(tmp.begin(), tmp.end(), [&](int a, int b) {
                if (combined[a].crowding != combined[b].crowding) return combined[a].crowding > combined[b].crowding;
                return combined[a].fitness < combined[b].fitness;
                });

            while (next.size() < P.popSize && !tmp.empty()) {
                next.push_back(combined[tmp.front()]);
                tmp.erase(tmp.begin());
            }
            break;
        }
    }

    auto f2 = nonDominatedSort(next);
    for (const auto& f : f2) calcCrowdingDistance(next, f);

    return next;
}


GP_Individual TreeEA::run() {
    std::vector<GP_Individual> pop;
    initPopulation(pop);

    pareto_.clear();
    pareto_.reserve(P.popSize * 2);
    for (const auto& ind : pop) updatePareto(ind);

    if (P.useNSGA2) {
        auto fronts0 = nonDominatedSort(pop);
        for (const auto& f : fronts0) calcCrowdingDistance(pop, f);
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

    GP_Individual bestSoFar = bestGen0_;


    for (size_t gen = 0; gen < P.generations; ++gen) {

        std::vector<GP_Individual> offspring;
        offspring.reserve(P.popSize);

        while (offspring.size() < P.popSize) {

            const GP_Individual& p1 = P.useNSGA2 ? tournamentMO(pop, P.tournamentK)
                : tournament(pop, P.tournamentK);
            const GP_Individual& p2 = P.useNSGA2 ? tournamentMO(pop, P.tournamentK)
                : tournament(pop, P.tournamentK);

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

            auto e1 = evaluate(c1);
            updatePareto(e1);
            offspring.push_back(e1);

            if (offspring.size() < P.popSize) {
                auto e2 = evaluate(c2);
                updatePareto(e2);
                offspring.push_back(e2);
            }
        }

        if (P.useNSGA2) {
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
    }


    return bestSoFar;
}
