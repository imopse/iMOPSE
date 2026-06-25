#include "GPTree.hpp"
#include "../rules/GPTreeRule.hpp"
#include "Features.hpp"
#include "../alloc/ResourceAllocator.hpp"
#include <sstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <cmath>
#include <functional>

double GPTree::featureValue(FeatureId id, const Features& f) const {
    switch (id) {
    case FeatureId::DURATION:              return f.duration;
    case FeatureId::REQ_LEVEL:             return f.reqLevel;
    case FeatureId::AVAIL_SKILL:           return f.availSkill;
    case FeatureId::CRITLEN:               return f.critLen;
    case FeatureId::SLACK:                 return f.slack;
    case FeatureId::DESC_COUNT:            return f.descCount;
    case FeatureId::TASK_RELEASE_PRESSURE: return f.taskReleasePressure;
    case FeatureId::TASK_CRITICAL_PRESSURE:return f.taskCriticalPressure;
    case FeatureId::AVAIL_GAP:             return f.availGap;
    case FeatureId::CHEAPEST_COST_NOW:     return f.cheapestCostNow;
    case FeatureId::COST_PER_SKILL_NOW:    return f.costPerSkillNow;
    case FeatureId::TASK_RES_COUNT:        return f.taskResCount;
    case FeatureId::AVG_RES_COST:          return f.avgResCostForSkill;
    case FeatureId::UNSCHED_TASKS:         return f.unschedTasks;
    case FeatureId::MIN_FEASIBLE_COST_NOW: return f.minFeasibleCostNow;
    case FeatureId::COST_REGRET_NOW:       return f.costRegretNow;

    case FeatureId::RES_WAGE:              return f.resWage;
    case FeatureId::RES_SKILL_LEVEL:       return f.resSkillLevel;
    case FeatureId::RES_IDLE_TIME:         return f.resIdleTime;
    case FeatureId::RES_CAN_START_NOW:     return f.resCanStartNow;
    case FeatureId::RES_UTILIZATION:       return f.resUtilization;
    case FeatureId::RES_WAGE_PER_LEVEL:    return f.resWagePerLevel;
    case FeatureId::RES_ASSIGN_COST:       return f.resAssignCost;
    case FeatureId::RES_ASSIGN_PREMIUM_ALL:return f.resAssignPremiumAll;
    case FeatureId::RES_RESERVE_PRESSURE:  return f.resReservePressure;
    case FeatureId::RES_FAMILY_MISMATCH:   return f.resFamilyMismatch;
    case FeatureId::RES_FUTURE_BRANCH_FIT: return f.resFutureBranchFit;
    case FeatureId::RES_BOTTLENECK_PRESERVATION: return f.resBottleneckPreservation;
    case FeatureId::RES_SPECIALIST_MISUSE: return f.resSpecialistMisuse;
    case FeatureId::RES_RELATIVE_WAGE:     return f.resRelativeWage;

    default:
        return 0.0;
    }
}


double GPTree::evalAt(int idx, const Features& f) const {
    if (idx < 0 || idx >= (int)nodes.size()) return 0.0;
    const GPNode& n = nodes[idx];
    switch (n.kind) {
    case NodeKind::CONST:   return n.constant;
    case NodeKind::FEATURE: return featureValue(n.feat, f);
    case NodeKind::UNARY: {
        double a = evalAt(n.left, f);
        switch (n.uop) {
        case UnaryOp::NEG: return pneg(a);
        case UnaryOp::ABS: return pabs(a);
        }
        return a;
    }
    case NodeKind::BINARY: {
        double a = evalAt(n.left, f);
        double b = evalAt(n.right, f);
        switch (n.bop) {
        case BinaryOp::ADD: return a + b;
        case BinaryOp::SUB: return a - b;
        case BinaryOp::MUL: return a * b;
        case BinaryOp::DIV: return pdiv(a, b);
        case BinaryOp::MIN: return pmin(a, b);
        case BinaryOp::MAX: return pmax(a, b);
        }
        return a;
    }
    }
    return 0.0;
}

double GPTree::eval(const Features& f) const {
    if (isEmpty()) return 0.0;
    double val = evalAt(root, f);
    if (!std::isfinite(val)) return 0.0;
    return val;
}

double GPTree::evalAtCollect(int idx, const Features& f, std::vector<double>& vals) const {
    if (idx < 0 || idx >= (int)nodes.size()) return 0.0;

    const GPNode& n = nodes[idx];
    double out = 0.0;

    switch (n.kind) {
    case NodeKind::CONST:
        out = n.constant;
        break;

    case NodeKind::FEATURE:
        out = featureValue(n.feat, f);
        break;

    case NodeKind::UNARY: {
        double a = evalAtCollect(n.left, f, vals);
        switch (n.uop) {
        case UnaryOp::NEG: out = pneg(a); break;
        case UnaryOp::ABS: out = pabs(a); break;
        default: out = a; break;
        }
        break;
    }

    case NodeKind::BINARY: {
        double a = evalAtCollect(n.left, f, vals);
        double b = evalAtCollect(n.right, f, vals);
        switch (n.bop) {
        case BinaryOp::ADD: out = a + b; break;
        case BinaryOp::SUB: out = a - b; break;
        case BinaryOp::MUL: out = a * b; break;
        case BinaryOp::DIV: out = pdiv(a, b); break;
        case BinaryOp::MIN: out = pmin(a, b); break;
        case BinaryOp::MAX: out = pmax(a, b); break;
        default: out = 0.0; break;
        }
        break;
    }
    }

    if (!std::isfinite(out)) out = 0.0;

    if ((size_t)idx >= vals.size()) {
        vals.resize(nodes.size(), 0.0);
    }
    vals[idx] = out;

    return out;
}

double GPTree::evalWithNodeValues(const Features& f, std::vector<double>* nodeValues) const {
    if (isEmpty()) {
        if (nodeValues) nodeValues->clear();
        return 0.0;
    }

    if (!nodeValues) {
        return eval(f);
    }

    nodeValues->assign(nodes.size(), 0.0);

    double val = evalAtCollect(root, f, *nodeValues);
    if (!std::isfinite(val)) val = 0.0;

    return val;
}

std::string GPTree::nodeLabel(int idx) const {
    if (idx < 0 || idx >= (int)nodes.size()) return "<?>";
    const GPNode& n = nodes[idx];
    std::ostringstream oss;
    switch (n.kind) {
    case NodeKind::CONST:
        oss << "CONST(" << std::setprecision(4) << n.constant << ")";
        break;

    case NodeKind::FEATURE: {
        const char* nm = "?";
        switch (n.feat) {
        case FeatureId::DURATION:              nm = "DUR";          break;
        case FeatureId::REQ_LEVEL:             nm = "REQ";          break;
        case FeatureId::AVAIL_SKILL:           nm = "AVAIL";        break;
        case FeatureId::CRITLEN:               nm = "CRITLEN";      break;
        case FeatureId::SLACK:                 nm = "SLACK";        break;
        case FeatureId::DESC_COUNT:            nm = "DESC_COUNT";   break;
        case FeatureId::TASK_RELEASE_PRESSURE: nm = "REL_PRESS";    break;
        case FeatureId::TASK_CRITICAL_PRESSURE:nm = "TASK_CRIT";    break;
        case FeatureId::AVAIL_GAP:             nm = "GAP";          break;
        case FeatureId::CHEAPEST_COST_NOW:     nm = "CHEAP";        break;
        case FeatureId::COST_PER_SKILL_NOW:    nm = "CHEAP_PER_SK"; break;
        case FeatureId::TASK_RES_COUNT:        nm = "TASK_RES";     break;
        case FeatureId::AVG_RES_COST:          nm = "AVG_RES_COST";     break;
        case FeatureId::UNSCHED_TASKS:         nm = "UNSCHED";      break;
        case FeatureId::MIN_FEASIBLE_COST_NOW: nm = "MIN_COST_NOW"; break;
        case FeatureId::COST_REGRET_NOW:       nm = "REGRET_NOW";   break;

        case FeatureId::RES_WAGE:              nm = "RES_WAGE";     break;
        case FeatureId::RES_SKILL_LEVEL:       nm = "RES_SKILL";    break;
        case FeatureId::RES_IDLE_TIME:         nm = "RES_IDLE";     break;
        case FeatureId::RES_CAN_START_NOW:     nm = "RES_CAN_NOW";  break;
        case FeatureId::RES_UTILIZATION:       nm = "RES_UTIL";     break;
        case FeatureId::RES_WAGE_PER_LEVEL:    nm = "RES_W_PER_L";  break;
        case FeatureId::RES_ASSIGN_COST:       nm = "RES_ASSIGN_COST";  break;
        case FeatureId::RES_ASSIGN_PREMIUM_ALL:nm = "RES_PREMIUM";   break;
        case FeatureId::RES_RESERVE_PRESSURE:  nm = "RES_RESERVE";    break;
        case FeatureId::RES_FAMILY_MISMATCH:   nm = "RES_FAM_MIS";    break;
        case FeatureId::RES_FUTURE_BRANCH_FIT: nm = "RES_FUT_BRANCH"; break;
        case FeatureId::RES_BOTTLENECK_PRESERVATION: nm = "RES_BOTTLENECK"; break;
        case FeatureId::RES_SPECIALIST_MISUSE: nm = "RES_SPEC_MIS";  break;
        case FeatureId::RES_RELATIVE_WAGE:     nm = "RES_REL_WAGE";  break;
        }
        oss << "FEAT:" << nm;
        break;
    }

    case NodeKind::UNARY:
        oss << (n.uop == UnaryOp::NEG ? "NEG" : "ABS");
        break;

    case NodeKind::BINARY: {
        const char* op = "?";
        switch (n.bop) {
        case BinaryOp::ADD: op = "+";   break;
        case BinaryOp::SUB: op = "-";   break;
        case BinaryOp::MUL: op = "*";   break;
        case BinaryOp::DIV: op = "/";   break;
        case BinaryOp::MIN: op = "min"; break;
        case BinaryOp::MAX: op = "max"; break;
        }
        oss << op;
        break;
    }
    }
    return oss.str();
}

std::string GPTree::toString() const {
    if (isEmpty()) return "CONST(0)";
    return toStringAt(root);
}

namespace {
    const char* featToString(FeatureId f) {
        switch (f) {
        case FeatureId::DURATION:              return "DUR";
        case FeatureId::REQ_LEVEL:             return "REQ";
        case FeatureId::AVAIL_SKILL:           return "AVAIL";
        case FeatureId::CRITLEN:               return "CRITLEN";
        case FeatureId::SLACK:                 return "SLACK";
        case FeatureId::DESC_COUNT:            return "DESC_COUNT";
        case FeatureId::TASK_RELEASE_PRESSURE: return "REL_PRESS";
        case FeatureId::TASK_CRITICAL_PRESSURE:return "TASK_CRIT";
        case FeatureId::AVAIL_GAP:             return "GAP";
        case FeatureId::CHEAPEST_COST_NOW:     return "CHEAP";
        case FeatureId::COST_PER_SKILL_NOW:    return "CHEAP_PER_SK";
        case FeatureId::TASK_RES_COUNT:        return "TASK_RES";
        case FeatureId::AVG_RES_COST:          return "AVG_RES_COST";
        case FeatureId::UNSCHED_TASKS:         return "UNSCHED";
        case FeatureId::MIN_FEASIBLE_COST_NOW: return "MIN_COST_NOW";
        case FeatureId::COST_REGRET_NOW:       return "REGRET_NOW";

        case FeatureId::RES_WAGE:              return "RES_WAGE";
        case FeatureId::RES_SKILL_LEVEL:       return "RES_SKILL";
        case FeatureId::RES_IDLE_TIME:         return "RES_IDLE";
        case FeatureId::RES_CAN_START_NOW:     return "RES_CAN_NOW";
        case FeatureId::RES_UTILIZATION:       return "RES_UTIL";
        case FeatureId::RES_WAGE_PER_LEVEL:    return "RES_W_PER_L";
        case FeatureId::RES_ASSIGN_COST:       return "RES_ASSIGN_COST";
        case FeatureId::RES_ASSIGN_PREMIUM_ALL:return "RES_PREMIUM";
        case FeatureId::RES_RESERVE_PRESSURE:  return "RES_RESERVE";
        case FeatureId::RES_FAMILY_MISMATCH:   return "RES_FAM_MIS";
        case FeatureId::RES_FUTURE_BRANCH_FIT: return "RES_FUT_BRANCH";
        case FeatureId::RES_BOTTLENECK_PRESERVATION: return "RES_BOTTLENECK";
        case FeatureId::RES_SPECIALIST_MISUSE: return "RES_SPEC_MIS";
        case FeatureId::RES_RELATIVE_WAGE:     return "RES_REL_WAGE";
        }
        return "?";
    }

    const char* bopToString(BinaryOp b) {
        switch (b) {
        case BinaryOp::ADD: return "ADD";
        case BinaryOp::SUB: return "SUB";
        case BinaryOp::MUL: return "MUL";
        case BinaryOp::DIV: return "DIV";
        case BinaryOp::MIN: return "MIN";
        case BinaryOp::MAX: return "MAX";
        }
        return "?";
    }
    const char* uopToString(UnaryOp u) {
        switch (u) {
        case UnaryOp::NEG: return "NEG";
        case UnaryOp::ABS: return "ABS";
        }
        return "?";
    }
}

std::string GPTree::toStringAt(int idx) const {
    if (idx < 0 || idx >= (int)nodes.size()) return "0";
    const GPNode& n = nodes[idx];
    std::ostringstream oss;

    switch (n.kind) {
    case NodeKind::CONST: {
        oss << "CONST(" << std::setprecision(6) << n.constant << ")";
        break;
    }
    case NodeKind::FEATURE: {
        oss << featToString(n.feat);
        break;
    }
    case NodeKind::UNARY: {
        const char* fn = (n.uop == UnaryOp::NEG ? "NEG" : "ABS");
        oss << fn << "(" << toStringAt(n.left) << ")";
        break;
    }
    case NodeKind::BINARY: {
        const char* op = nullptr;
        switch (n.bop) {
        case BinaryOp::ADD: op = "+";   break;
        case BinaryOp::SUB: op = "-";   break;
        case BinaryOp::MUL: op = "*";   break;
        case BinaryOp::DIV: op = "/";   break;
        case BinaryOp::MIN: op = "min"; break;
        case BinaryOp::MAX: op = "max"; break;
        }
        if (n.bop == BinaryOp::MIN || n.bop == BinaryOp::MAX) {
            oss << op << "(" << toStringAt(n.left) << "," << toStringAt(n.right) << ")";
        }
        else {
            oss << "(" << toStringAt(n.left) << " " << op << " " << toStringAt(n.right) << ")";
        }
        break;
    }
    }
    return oss.str();
}


namespace gp { struct CPMPrecalc; }
void setCPMPrecalc(const gp::CPMPrecalc* p);

static UnaryOp sampleU(std::mt19937& rng) {
    std::uniform_int_distribution<int> U(0, 1);
    return U(rng) ? UnaryOp::NEG : UnaryOp::ABS;
}

static BinaryOp sampleB(std::mt19937& rng) {
    static const BinaryOp ops[] = {
        BinaryOp::ADD,
        BinaryOp::SUB,
        BinaryOp::MUL,
        BinaryOp::DIV,
        BinaryOp::MIN,
        BinaryOp::MAX
    };
    std::uniform_int_distribution<int> U(0, 5);
    return ops[U(rng)];
}

static FeatureId sampleFeatMS(std::mt19937& rng) {
    const auto pool = GPTree::allTaskFeatures();
    std::uniform_int_distribution<int> U(0, (int)pool.size() - 1);
    return pool[U(rng)];
}

static FeatureId sampleFeatRES(std::mt19937& rng) {
    const auto pool = GPTree::allResFeatures();
    std::uniform_int_distribution<int> U(0, (int)pool.size() - 1);
    return pool[U(rng)];
}

static FeatureId sampleFeatPAIR(std::mt19937& rng) {
    const auto pool = GPTree::allPairFeatures();
    std::uniform_int_distribution<int> U(0, (int)pool.size() - 1);
    return pool[U(rng)];
}

static int growMS(std::mt19937& rng, std::vector<GPNode>& v, int depth, int maxDepth) {
    std::uniform_real_distribution<double> U01(0.0, 1.0);

    if (depth == maxDepth || U01(rng) < 0.25) {
        if (U01(rng) < 0.7) {
            GPNode f; f.kind = NodeKind::FEATURE; f.feat = sampleFeatMS(rng);
            return GPTree::add(v, f);
        }
        GPNode c; c.kind = NodeKind::CONST;
        c.constant = (U01(rng) * 2.0 - 1.0);
        return GPTree::add(v, c);
    }

    GPNode b; b.kind = NodeKind::BINARY;
    b.bop = sampleB(rng);
    b.left = growMS(rng, v, depth + 1, maxDepth);
    b.right = growMS(rng, v, depth + 1, maxDepth);
    return GPTree::add(v, b);
}

static int growRES(std::mt19937& rng, std::vector<GPNode>& v, int depth, int maxDepth) {
    std::uniform_real_distribution<double> U01(0.0, 1.0);

    if (depth == maxDepth || U01(rng) < 0.25) {
        if (U01(rng) < 0.7) {
            GPNode f; f.kind = NodeKind::FEATURE; f.feat = sampleFeatRES(rng);
            return GPTree::add(v, f);
        }
        GPNode c; c.kind = NodeKind::CONST;
        c.constant = (U01(rng) * 2.0 - 1.0) * 10.0;
        return GPTree::add(v, c);
    }

    GPNode b; b.kind = NodeKind::BINARY;
    b.bop = sampleB(rng);
    b.left = growRES(rng, v, depth + 1, maxDepth);
    b.right = growRES(rng, v, depth + 1, maxDepth);
    return GPTree::add(v, b);
}

static int growPAIR(std::mt19937& rng, std::vector<GPNode>& v, int depth, int maxDepth) {
    std::uniform_real_distribution<double> U01(0.0, 1.0);

    if (depth == maxDepth || U01(rng) < 0.25) {
        if (U01(rng) < 0.7) {
            GPNode f; f.kind = NodeKind::FEATURE; f.feat = sampleFeatPAIR(rng);
            return GPTree::add(v, f);
        }
        GPNode c; c.kind = NodeKind::CONST;
        c.constant = (U01(rng) * 2.0 - 1.0);
        return GPTree::add(v, c);
    }

    GPNode b; b.kind = NodeKind::BINARY;
    b.bop = sampleB(rng);
    b.left = growPAIR(rng, v, depth + 1, maxDepth);
    b.right = growPAIR(rng, v, depth + 1, maxDepth);
    return GPTree::add(v, b);
}

GPTree GPTree::RandomTreeMS(std::mt19937& rng, int maxDepth) {
    GPTree t;
    do {
        t.nodes.clear();
        t.root = growMS(rng, t.nodes, 0, maxDepth);
    } while (!t.hasAnyFeature());
    return t;
}

GPTree GPTree::RandomTreeRES(std::mt19937& rng, int maxDepth) {
    GPTree t;
    do {
        t.nodes.clear();
        t.root = growRES(rng, t.nodes, 0, maxDepth);
    } while (!t.hasAnyFeature());
    return t;
}

GPTree GPTree::RandomTreePAIR(std::mt19937& rng, int maxDepth) {
    GPTree t;
    do {
        t.nodes.clear();
        t.root = growPAIR(rng, t.nodes, 0, maxDepth);
    } while (!t.hasAnyFeature());
    return t;
}

GPTree GPTree::Make_AVAIL_minus_REQ() {
    GPTree t;
    GPNode av; av.kind = NodeKind::FEATURE; av.feat = FeatureId::AVAIL_SKILL; int ax = GPTree::add(t.nodes, av);
    GPNode rq; rq.kind = NodeKind::FEATURE; rq.feat = FeatureId::REQ_LEVEL;   int rx = GPTree::add(t.nodes, rq);
    GPNode sub; sub.kind = NodeKind::BINARY; sub.bop = BinaryOp::SUB; sub.left = ax; sub.right = rx;
    t.root = GPTree::add(t.nodes, sub);
    return t;
}

GPTree GPTree::Make_REQ_times_DUR() {
    GPTree t;
    GPNode rq; rq.kind = NodeKind::FEATURE; rq.feat = FeatureId::REQ_LEVEL;   int rx = GPTree::add(t.nodes, rq);
    GPNode du; du.kind = NodeKind::FEATURE; du.feat = FeatureId::DURATION;    int dx = GPTree::add(t.nodes, du);
    GPNode mul; mul.kind = NodeKind::BINARY; mul.bop = BinaryOp::MUL; mul.left = rx; mul.right = dx;
    t.root = GPTree::add(t.nodes, mul);
    return t;
}

GPTree GPTree::Make_CHEAPxDUR() {
    GPTree t;
    GPNode l; l.kind = NodeKind::FEATURE; l.feat = FeatureId::CHEAPEST_COST_NOW;
    int lx = (int)t.nodes.size(); t.nodes.push_back(l);
    GPNode r; r.kind = NodeKind::FEATURE; r.feat = FeatureId::DURATION;
    int rx = (int)t.nodes.size(); t.nodes.push_back(r);
    GPNode m; m.kind = NodeKind::BINARY; m.bop = BinaryOp::MUL; m.left = lx; m.right = rx;
    t.root = (int)t.nodes.size(); t.nodes.push_back(m);
    return t;
}

GPTree GPTree::Make_CHEAP_PER_SKILL_plus_DUR() {
    GPTree t;
    GPNode a; a.kind = NodeKind::FEATURE; a.feat = FeatureId::COST_PER_SKILL_NOW;
    int ax = (int)t.nodes.size(); t.nodes.push_back(a);
    GPNode b; b.kind = NodeKind::FEATURE; b.feat = FeatureId::DURATION;
    int bx = (int)t.nodes.size(); t.nodes.push_back(b);
    GPNode add; add.kind = NodeKind::BINARY; add.bop = BinaryOp::ADD; add.left = ax; add.right = bx;
    t.root = (int)t.nodes.size(); t.nodes.push_back(add);
    return t;
}

GPTree GPTree::Make_BNTGA_BridgeRatioPair() {
    GPTree t;

    auto feat = [&](FeatureId f) -> int {
        GPNode n{};
        n.kind = NodeKind::FEATURE;
        n.feat = f;
        return GPTree::add(t.nodes, n);
        };

    auto cst = [&](double v) -> int {
        GPNode n{};
        n.kind = NodeKind::CONST;
        n.constant = v;
        return GPTree::add(t.nodes, n);
        };

    auto bin = [&](BinaryOp op, int l, int r) -> int {
        GPNode n{};
        n.kind = NodeKind::BINARY;
        n.bop = op;
        n.left = l;
        n.right = r;
        return GPTree::add(t.nodes, n);
        };

    auto un = [&](UnaryOp op, int child) -> int {
        GPNode n{};
        n.kind = NodeKind::UNARY;
        n.uop = op;
        n.left = child;
        return GPTree::add(t.nodes, n);
        };

    // Features
    int dur = feat(FeatureId::DURATION);
    int slack = feat(FeatureId::SLACK);
    int critLen = feat(FeatureId::CRITLEN);
    int descCount = feat(FeatureId::DESC_COUNT);

    int req = feat(FeatureId::REQ_LEVEL);
    int resSkill = feat(FeatureId::RES_SKILL_LEVEL);
    int resRelWage = feat(FeatureId::RES_RELATIVE_WAGE);
    int famMis = feat(FeatureId::RES_FAMILY_MISMATCH);
    int futBranch = feat(FeatureId::RES_FUTURE_BRANCH_FIT);

    int taskCrit = feat(FeatureId::TASK_CRITICAL_PRESSURE);
    int relPress = feat(FeatureId::TASK_RELEASE_PRESSURE);
    int regretNow = feat(FeatureId::COST_REGRET_NOW);
    int resCanNow = feat(FeatureId::RES_CAN_START_NOW);

    // Constants
    int c015 = cst(0.15);
    int c003 = cst(0.03);
    int c018 = cst(0.18);

    int c300 = cst(3.00);
    int c010 = cst(0.10);
    int c050 = cst(0.50);
    int c012 = cst(0.12);
    int c120 = cst(1.20);
    int c110 = cst(1.10);
    int c095 = cst(0.95);

    // Left side: lekkie kary lokalne
    int skillGap = bin(BinaryOp::SUB, resSkill, req);
    int absSkillGap = un(UnaryOp::ABS, skillGap);
    int fitPenalty = bin(BinaryOp::MUL, c015, absSkillGap);

    int wagePenalty = bin(BinaryOp::MUL, c003, resRelWage);
    int famPenalty = bin(BinaryOp::MUL, c018, famMis);

    int leftA = bin(BinaryOp::ADD, fitPenalty, wagePenalty);
    int leftPart = bin(BinaryOp::ADD, leftA, famPenalty);

    // Right side: time core + gate + future corridor
    int criticalCore = bin(BinaryOp::MAX, taskCrit, relPress);
    int durationBias = bin(BinaryOp::ADD, dur, c050);
    int bridgeRaw = bin(BinaryOp::MUL, criticalCore, durationBias);
    int bridgeScaled = bin(BinaryOp::MUL, c300, bridgeRaw);

    int denomA = bin(BinaryOp::ADD, c010, slack);
    int denom = bin(BinaryOp::ADD, denomA, critLen);

    int bridgeUrgency = bin(BinaryOp::DIV, bridgeScaled, denom);

    int gateBonus = bin(BinaryOp::MUL, c012, descCount);
    int regretBoost = bin(BinaryOp::MUL, c120, regretNow);
    int futureBoost = bin(BinaryOp::MUL, c110, futBranch);
    int canNowBoost = bin(BinaryOp::MUL, c095, resCanNow);

    int rewardA = bin(BinaryOp::ADD, bridgeUrgency, gateBonus);
    int rewardB = bin(BinaryOp::ADD, rewardA, regretBoost);
    int rewardC = bin(BinaryOp::ADD, rewardB, futureBoost);
    int reward = bin(BinaryOp::ADD, rewardC, canNowBoost);

    int rootExpr = bin(BinaryOp::SUB, leftPart, reward);

    t.root = rootExpr;
    return t;
}

int GPTree::depthAt(int idx) const {
    if (idx < 0 || idx >= (int)nodes.size()) return 0;
    const GPNode& n = nodes[idx];
    switch (n.kind) {
    case NodeKind::CONST:
    case NodeKind::FEATURE:
        return 1;
    case NodeKind::UNARY:
        return 1 + depthAt(n.left);
    case NodeKind::BINARY:
        return 1 + std::max(depthAt(n.left), depthAt(n.right));
    }
    return 1;
}

int GPTree::depth() const {
    if (root < 0 || root >= (int)nodes.size()) return 0;
    return depthAt(root);
}

int GPTree::nodeDepth(int nodeId) const {
    if (root < 0 || root >= (int)nodes.size()) return 0;
    if (nodeId == root) return 0;
    std::vector<int> q{ root };
    std::vector<int> d(nodes.size(), -1);
    d[root] = 0;
    for (size_t i = 0; i < q.size(); ++i) {
        int u = q[i];
        const GPNode& n = nodes[u];
        auto try_push = [&](int v) {
            if (v >= 0 && v < (int)nodes.size() && d[v] == -1) {
                d[v] = d[u] + 1; q.push_back(v);
            }
            };
        if (n.kind == NodeKind::UNARY)  try_push(n.left);
        if (n.kind == NodeKind::BINARY) { try_push(n.left); try_push(n.right); }
    }
    return (nodeId >= 0 && nodeId < (int)nodes.size() && d[nodeId] >= 0) ? d[nodeId] : 0;
}

int GPTree::subtreeHeight(int index) const {
    return depthAt(index);
}


std::vector<FeatureId> GPTree::allTaskFeatures() {
    return {
         FeatureId::DURATION,
         FeatureId::REQ_LEVEL,
         FeatureId::AVAIL_SKILL,
         FeatureId::CRITLEN,
         FeatureId::SLACK,
         FeatureId::DESC_COUNT,
         FeatureId::TASK_CRITICAL_PRESSURE,
         FeatureId::TASK_RELEASE_PRESSURE,
         FeatureId::AVAIL_GAP,
         FeatureId::CHEAPEST_COST_NOW,
         FeatureId::COST_PER_SKILL_NOW,
         FeatureId::TASK_RES_COUNT,
         FeatureId::AVG_RES_COST,
         FeatureId::UNSCHED_TASKS,
         FeatureId::MIN_FEASIBLE_COST_NOW,
         FeatureId::COST_REGRET_NOW
    };
}

std::vector<FeatureId> GPTree::allResFeatures() {
    return {
        FeatureId::RES_WAGE,
        FeatureId::RES_SKILL_LEVEL,
        FeatureId::RES_IDLE_TIME,
        FeatureId::RES_CAN_START_NOW,
        FeatureId::RES_UTILIZATION,
        FeatureId::RES_WAGE_PER_LEVEL,
        FeatureId::RES_ASSIGN_COST,
        FeatureId::RES_ASSIGN_PREMIUM_ALL,
        FeatureId::RES_RESERVE_PRESSURE,
        FeatureId::RES_FAMILY_MISMATCH,
        FeatureId::RES_FUTURE_BRANCH_FIT,
        FeatureId::RES_BOTTLENECK_PRESERVATION,
        FeatureId::RES_SPECIALIST_MISUSE,
        FeatureId::RES_RELATIVE_WAGE
    };
}

std::vector<FeatureId> GPTree::allPairFeatures() {
    std::vector<FeatureId> v = allTaskFeatures();
    auto r = allResFeatures();
    v.insert(v.end(), r.begin(), r.end());

    return v;
}

int GPTree::cloneSubtreeDFS(int nodeId, std::vector<int>& order) const {
    if (nodeId < 0 || nodeId >= (int)nodes.size()) return 0;
    order.push_back(nodeId);
    const GPNode& n = nodes[nodeId];
    if (n.kind == NodeKind::UNARY)  cloneSubtreeDFS(n.left, order);
    if (n.kind == NodeKind::BINARY) { cloneSubtreeDFS(n.left, order); cloneSubtreeDFS(n.right, order); }
    return 0;
}

GPTree GPTree::extractSubtree(int nodeId) const {
    GPTree out;
    if (nodeId < 0 || nodeId >= (int)nodes.size()) return out;
    std::vector<int> order;
    cloneSubtreeDFS(nodeId, order);

    std::vector<int> mapIdx(nodes.size(), -1);
    for (size_t i = 0; i < order.size(); ++i) mapIdx[order[i]] = (int)i;

    out.nodes.resize(order.size());
    for (size_t i = 0; i < order.size(); ++i) {
        int oldIdx = order[i];
        GPNode n = nodes[oldIdx];
        if (n.kind == NodeKind::UNARY) {
            n.left = (n.left >= 0 ? mapIdx[n.left] : -1);
        }
        else if (n.kind == NodeKind::BINARY) {
            n.left = (n.left >= 0 ? mapIdx[n.left] : -1);
            n.right = (n.right >= 0 ? mapIdx[n.right] : -1);
        }
        out.nodes[(int)i] = n;
    }
    out.root = 0;
    return out;
}

GPTree GPTree::graftedWith(int replaceIndex, const GPTree& donor) const {
    if (isEmpty()) return GPTree{ *this };
    if (donor.isEmpty()) return GPTree{ *this };

    GPTree out;
    out.nodes.reserve(nodes.size() + donor.nodes.size());

    std::vector<int> mapHost(nodes.size(), -1);
    std::vector<int> mapDonor(donor.nodes.size(), -1);

    std::function<int(int)> cloneDonor = [&](int u) -> int {
        if (u < 0) return -1;
        int& m = mapDonor[u];
        if (m != -1) return m;

        GPNode n = donor.nodes[u];
        if (n.kind == NodeKind::CONST || n.kind == NodeKind::FEATURE) {
            n.left = n.right = -1;
        }
        else if (n.kind == NodeKind::UNARY) {
            int L = cloneDonor(n.left);
            n.left = L; n.right = -1;
        }
        else {
            int L = cloneDonor(n.left);
            int R = cloneDonor(n.right);
            n.left = L; n.right = R;
        }

        m = (int)out.nodes.size();
        out.nodes.push_back(n);
        return m;
        };

    std::function<int(int)> cloneHost = [&](int u) -> int {
        if (u < 0) return -1;
        if (u == replaceIndex) {
            return cloneDonor(donor.root);
        }
        int& m = mapHost[u];
        if (m != -1) return m;

        GPNode n = nodes[u];
        if (n.kind == NodeKind::CONST || n.kind == NodeKind::FEATURE) {
            n.left = n.right = -1;
        }
        else if (n.kind == NodeKind::UNARY) {
            int L = cloneHost(n.left);
            n.left = L; n.right = -1;
        }
        else {
            int L = cloneHost(n.left);
            int R = cloneHost(n.right);
            n.left = L; n.right = R;
        }

        m = (int)out.nodes.size();
        out.nodes.push_back(n);
        return m;
        };

    int newRoot = cloneHost(root);
    out.root = newRoot;
    return out;
}

bool GPTree::isStructurallySound() const {
    if (isEmpty()) return false;
    const int N = (int)nodes.size();
    if (root < 0 || root >= N) return false;
    if (N > 200000) return false;

    auto inRange = [&](int x) { return x >= -1 && x < N; };

    for (int i = 0; i < N; ++i) {
        const GPNode& n = nodes[i];
        switch (n.kind) {
        case NodeKind::CONST:
        case NodeKind::FEATURE:
            if (n.left != -1 || n.right != -1) return false;
            break;
        case NodeKind::UNARY:
            if (!inRange(n.left) || n.right != -1) return false;
            break;
        case NodeKind::BINARY:
            if (!inRange(n.left) || !inRange(n.right)) return false;
            break;
        }
    }

    std::vector<char> state(N, 0);
    std::function<bool(int)> dfs = [&](int u) -> bool {
        if (u < 0 || u >= N) return false;
        if (state[u] == 1) return false;
        if (state[u] == 2) return true;
        state[u] = 1;
        const GPNode& n = nodes[u];
        if (n.kind == NodeKind::UNARY) {
            if (!dfs(n.left)) return false;
        }
        else if (n.kind == NodeKind::BINARY) {
            if (!dfs(n.left))  return false;
            if (!dfs(n.right)) return false;
        }
        state[u] = 2;
        return true;
        };
    if (!dfs(root)) return false;

    for (int i = 0; i < N; ++i) if (state[i] != 2) return false;

    return true;
}

int GPTree::rebuildWithReplace(int nodeId, const GPTree* sub, int replaceAt, std::vector<GPNode>& out) const {
    if (nodeId < 0 || nodeId >= (int)nodes.size()) return -1;

    if (nodeId == replaceAt && sub) {
        int base = (int)out.size();
        out.insert(out.end(), sub->nodes.begin(), sub->nodes.end());
        return base + sub->root;
    }

    const GPNode& n = nodes[nodeId];
    GPNode copy = n;
    int myIndex = (int)out.size();
    out.push_back(copy);

    if (n.kind == NodeKind::UNARY) {
        int ch = rebuildWithReplace(n.left, sub, replaceAt, out);
        out[myIndex].left = ch;
    }
    else if (n.kind == NodeKind::BINARY) {
        int l = rebuildWithReplace(n.left, sub, replaceAt, out);
        int r = rebuildWithReplace(n.right, sub, replaceAt, out);
        out[myIndex].left = l;
        out[myIndex].right = r;
    }
    return myIndex;
}

void GPTree::replaceSubtree(int nodeId, const GPTree& sub) {
    GPTree out = this->graftedWith(nodeId, sub);
    if (!out.isStructurallySound()) return;

    *this = std::move(out);
}

