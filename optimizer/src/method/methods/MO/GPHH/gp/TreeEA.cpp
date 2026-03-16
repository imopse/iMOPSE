#include "TreeEA.hpp"
#include "../rules/GPTreeRule.hpp"
#include "../rules/GPTreeResRule.hpp"
#include "problem/problems/MSRCPSP/CScheduler.h"
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
        FeatureId::RES_WAIT_TIME,
        FeatureId::RES_IDLE_TIME,
        FeatureId::RES_CAN_START_NOW,
        FeatureId::RES_MULTI_SKILL,
        FeatureId::RES_UTILIZATION,
        FeatureId::RES_WAGE_PER_LEVEL,
        FeatureId::RES_ASSIGN_COST,
        FeatureId::RES_ASSIGN_PREMIUM_ALL,
        FeatureId::RES_HASTE_VALUE,
        FeatureId::RES_RESERVE_PRESSURE,
        FeatureId::RES_STRATEGIC_MISMATCH,
        FeatureId::RES_FAMILY_MISMATCH,
        FeatureId::RES_SURPLUS_LEVEL,
        FeatureId::RES_RELATIVE_WAGE,
        FeatureId::RES_FUTURE_DEMAND
    };
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
        t.nodes[i].bop = static_cast<BinaryOp>(randInt(0, 5));

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

        for (size_t gen = 0; gen < P.generations; ++gen) {

            std::vector<GP_Individual> offspring;
            offspring.reserve(P.popSize);

            auto pairs = selectParentsBNTGA(/*objectiveNumber=*/2, (int)P.popSize);

            const std::vector<GP_Individual> archiveSnap = archive_;

            for (auto [ia, ib] : pairs) {

                if (ia < 0 || ib < 0 || ia >= (int)archiveSnap.size() || ib >= (int)archiveSnap.size()) {
                    continue;
                }

                GP_Individual c1 = archiveSnap[ia];
                GP_Individual c2 = archiveSnap[ib];

                if (rand01() < P.pCrossover) crossover(c1.taskTree, c2.taskTree, false);
                if (rand01() < P.pCrossover) crossover(c1.resTree, c2.resTree, true);

                if (rand01() < P.pMutParam)  mutateParam(c1.taskTree, false);
                if (rand01() < P.pMutParam)  mutateParam(c2.taskTree, false);
                if (rand01() < P.pMutParam)  mutateParam(c1.resTree, true);
                if (rand01() < P.pMutParam)  mutateParam(c2.resTree, true);

                auto structOrMacro = [&](GPTree& tr, bool isRes) {
                    if (rand01() < P.pMutMacroSubtree) mutateMacroSubtreeReplace(tr, isRes);
                    else if (rand01() < P.pMutStruct)  mutateStruct(tr, isRes);
                    };

                structOrMacro(c1.taskTree, false);
                structOrMacro(c2.taskTree, false);
                structOrMacro(c1.resTree, true);
                structOrMacro(c2.resTree, true);

                auto e1 = evaluate(c1);
                offspring.push_back(e1);

                if (offspring.size() < P.popSize) {
                    auto e2 = evaluate(c2);
                    offspring.push_back(e2);
                }
            }

            copyToArchiveWithFiltering(offspring);

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

            auto structOrMacro = [&](GPTree& tr, bool isRes) {
                if (rand01() < P.pMutMacroSubtree) mutateMacroSubtreeReplace(tr, isRes);
                else if (rand01() < P.pMutStruct)  mutateStruct(tr, isRes);
                };

            structOrMacro(c1.taskTree, false);
            structOrMacro(c2.taskTree, false);
            structOrMacro(c1.resTree, true);
            structOrMacro(c2.resTree, true);

            auto e1 = evaluate(c1);
            if (!P.useNSGA2) updatePareto(e1);
            offspring.push_back(e1);

            if (offspring.size() < P.popSize) {
                auto e2 = evaluate(c2);
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
    }

    return bestSoFar;
}
