#include "gp/GPTree.hpp"
#include "rules/GPTreeRule.hpp"
#include "gp/Features.hpp"
#include "alloc/ResourceAllocator.hpp"
#include <sstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <cmath>
#include <functional>

double GPTree::featureValue(FeatureId id, const Features& f) const {
    switch (id) {
    case FeatureId::DURATION:             return f.duration;
    case FeatureId::REQ_LEVEL:            return f.reqLevel;
    case FeatureId::AVAIL_SKILL:          return f.availSkill;
    case FeatureId::EST_PREC:             return f.estPrec;
    case FeatureId::SUCC_COUNT:           return f.succCount;
    case FeatureId::CRITLEN:              return f.critLen;
    case FeatureId::SLACK:                return f.slack;
    case FeatureId::AVAIL_GAP:            return f.availGap;
    case FeatureId::WAIT_RES:             return f.waitRes;
    case FeatureId::TOT_PRED:             return f.totPred;
    case FeatureId::CHEAPEST_COST_NOW:    return f.cheapestCostNow;
    case FeatureId::COST_PER_SKILL_NOW:   return f.costPerSkillNow;
    case FeatureId::MIN_WAGE_AVAIL:       return f.minWageAvail;
    case FeatureId::AVG_WAGE_AVAIL:       return f.avgWageAvail;
    case FeatureId::TEAM_SIZE_MIN_NOW:    return f.teamSizeMinNow;
    case FeatureId::NUM_TASKS:       return f.numTasks;
    case FeatureId::NUM_RESOURCES:   return f.numResources;
    case FeatureId::NUM_SKILLS:      return f.numSkills;
    case FeatureId::TASK_RES_COUNT:  return f.taskResCount;
    case FeatureId::AVG_RES_COST:    return f.avgResCostForSkill;
    case FeatureId::UNSCHED_TASKS:   return f.unschedTasks;
    case FeatureId::RES_WAGE:             return f.resWage;
    case FeatureId::RES_SKILL_LEVEL:      return f.resSkillLevel;
    case FeatureId::RES_FREE_TIME:        return f.resFreeTime;
    case FeatureId::RES_MULTI_SKILL:      return f.resMultiSkill;
    case FeatureId::RES_UTILIZATION:      return f.resUtilization;
    }
    return 0.0;
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
        case FeatureId::DURATION:            nm = "DUR";        break;
        case FeatureId::REQ_LEVEL:           nm = "REQ";        break;
        case FeatureId::AVAIL_SKILL:         nm = "AVAIL";      break;
        case FeatureId::EST_PREC:            nm = "EST";        break;
        case FeatureId::SUCC_COUNT:          nm = "SUCC";       break;
        case FeatureId::CRITLEN:             nm = "CRITLEN";    break;
        case FeatureId::SLACK:               nm = "SLACK";      break;
        case FeatureId::AVAIL_GAP:           nm = "GAP";        break;
        case FeatureId::WAIT_RES:            nm = "WAIT";       break;
        case FeatureId::TOT_PRED:            nm = "TPRED";      break;
        case FeatureId::CHEAPEST_COST_NOW:   nm = "CHEAP";      break;
        case FeatureId::COST_PER_SKILL_NOW:  nm = "CHEAP_PER_SK"; break;
        case FeatureId::MIN_WAGE_AVAIL:      nm = "MIN_WAGE";   break;
        case FeatureId::AVG_WAGE_AVAIL:      nm = "AVG_WAGE";   break;
        case FeatureId::TEAM_SIZE_MIN_NOW:   nm = "TEAM_SIZE";  break;
        case FeatureId::NUM_TASKS:        nm = "N_TASKS";    break;
        case FeatureId::NUM_RESOURCES:    nm = "N_RES";      break;
        case FeatureId::NUM_SKILLS:       nm = "N_SKILLS";   break;
        case FeatureId::TASK_RES_COUNT:   nm = "TASK_RES";   break;
        case FeatureId::AVG_RES_COST:     nm = "RES_COST";   break;
        case FeatureId::UNSCHED_TASKS:    nm = "UNSCHED";    break;
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
        case FeatureId::DURATION:            return "DUR";
        case FeatureId::REQ_LEVEL:           return "REQ";
        case FeatureId::AVAIL_SKILL:         return "AVAIL";
        case FeatureId::EST_PREC:            return "EST";
        case FeatureId::SUCC_COUNT:          return "SUCC";
        case FeatureId::CRITLEN:             return "CRITLEN";
        case FeatureId::SLACK:               return "SLACK";
        case FeatureId::AVAIL_GAP:           return "GAP";
        case FeatureId::WAIT_RES:            return "WAIT";
        case FeatureId::TOT_PRED:            return "TPRED";
        case FeatureId::CHEAPEST_COST_NOW:   return "CHEAP";
        case FeatureId::COST_PER_SKILL_NOW:  return "CHEAP_PER_SK";
        case FeatureId::MIN_WAGE_AVAIL:      return "MIN_WAGE";
        case FeatureId::AVG_WAGE_AVAIL:      return "AVG_WAGE";
        case FeatureId::TEAM_SIZE_MIN_NOW:   return "TEAM_SIZE";
        case FeatureId::NUM_TASKS:      return "N_TASKS";
        case FeatureId::NUM_RESOURCES:  return "N_RES";
        case FeatureId::NUM_SKILLS:     return "N_SKILLS";
        case FeatureId::TASK_RES_COUNT: return "TASK_RES";
        case FeatureId::AVG_RES_COST:   return "RES_COST";
        case FeatureId::UNSCHED_TASKS:  return "UNSCHED";
        case FeatureId::RES_WAGE:            return "RES_WAGE";
        case FeatureId::RES_SKILL_LEVEL:     return "RES_SKILL";
        case FeatureId::RES_FREE_TIME:       return "RES_FREE";
        case FeatureId::RES_MULTI_SKILL:     return "RES_MULTI";
        case FeatureId::RES_UTILIZATION:     return "RES_UTIL";
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

std::string GPTree::toJson() const {
    std::ostringstream oss;
    oss.imbue(std::locale::classic());
    oss << std::setprecision(12);

    oss << "{";
    oss << "\"root\":" << this->root << ",\"nodes\":[";
    for (size_t i = 0; i < nodes.size(); ++i) {
        const GPNode& n = nodes[i];
        if (i) oss << ",";
        oss << "{";

        switch (n.kind) {
        case NodeKind::CONST: {
            double v = n.constant;
            if (!std::isfinite(v)) v = 0.0;
            oss << "\"kind\":\"CONST\",\"constant\":" << v;
            break;
        }
        case NodeKind::FEATURE: {
            oss << "\"kind\":\"FEATURE\",\"feat\":\"" << featToString(n.feat) << "\"";
            break;
        }
        case NodeKind::UNARY: {
            oss << "\"kind\":\"UNARY\",\"uop\":\"" << uopToString(n.uop)
                << "\",\"left\":" << n.left;
            break;
        }
        case NodeKind::BINARY: {
            oss << "\"kind\":\"BINARY\",\"bop\":\"" << bopToString(n.bop)
                << "\",\"left\":" << n.left << ",\"right\":" << n.right;
            break;
        }
        }

        oss << "}";
    }
    oss << "]}";
    return oss.str();
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
        const char* nm = "?";
        switch (n.feat) {
        case FeatureId::DURATION:            nm = "DUR";        break;
        case FeatureId::REQ_LEVEL:           nm = "REQ";        break;
        case FeatureId::AVAIL_SKILL:         nm = "AVAIL";      break;
        case FeatureId::EST_PREC:            nm = "EST";        break;
        case FeatureId::SUCC_COUNT:          nm = "SUCC";       break;
        case FeatureId::CRITLEN:             nm = "CRITLEN";    break;
        case FeatureId::SLACK:               nm = "SLACK";      break;
        case FeatureId::AVAIL_GAP:           nm = "GAP";        break;
        case FeatureId::WAIT_RES:            nm = "WAIT";       break;
        case FeatureId::TOT_PRED:            nm = "TPRED";      break;
        case FeatureId::CHEAPEST_COST_NOW:   nm = "CHEAP";      break;
        case FeatureId::COST_PER_SKILL_NOW:  nm = "CHEAP_PER_SK"; break;
        case FeatureId::MIN_WAGE_AVAIL:      nm = "MIN_WAGE";   break;
        case FeatureId::AVG_WAGE_AVAIL:      nm = "AVG_WAGE";   break;
        case FeatureId::TEAM_SIZE_MIN_NOW:   nm = "TEAM_SIZE";  break;
        case FeatureId::NUM_TASKS:      nm = "N_TASKS";   break;
        case FeatureId::NUM_RESOURCES:  nm = "N_RES";     break;
        case FeatureId::NUM_SKILLS:     nm = "N_SKILLS";  break;
        case FeatureId::TASK_RES_COUNT: nm = "TASK_RES";  break;
        case FeatureId::AVG_RES_COST:   nm = "RES_COST";  break;
        case FeatureId::UNSCHED_TASKS:  nm = "UNSCHED";   break;

        case FeatureId::RES_WAGE:         nm = "RES_WAGE";  break;
        case FeatureId::RES_SKILL_LEVEL:  nm = "RES_SKILL"; break;
        case FeatureId::RES_FREE_TIME:    nm = "RES_FREE";  break;
        case FeatureId::RES_MULTI_SKILL:  nm = "RES_MULTI"; break;
        case FeatureId::RES_UTILIZATION:  nm = "RES_UTIL";  break;

        }
        oss << nm;
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
        case BinaryOp::ADD: op = "+";  break;
        case BinaryOp::SUB: op = "-";  break;
        case BinaryOp::MUL: op = "*";  break;
        case BinaryOp::DIV: op = "/";  break;
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
    std::uniform_int_distribution<int> U(0, 5);
    return static_cast<BinaryOp>(U(rng));
}

static FeatureId sampleFeatMS(std::mt19937& rng) {
    std::uniform_int_distribution<int> U(0, 9);
    switch (U(rng)) {
    case 0: return FeatureId::DURATION;
    case 1: return FeatureId::REQ_LEVEL;
    case 2: return FeatureId::AVAIL_SKILL;
    case 3: return FeatureId::EST_PREC;
    case 4: return FeatureId::SUCC_COUNT;
    case 5: return FeatureId::CRITLEN;
    case 6: return FeatureId::SLACK;
    case 7: return FeatureId::AVAIL_GAP;
    case 8: return FeatureId::WAIT_RES;
    default:return FeatureId::TOT_PRED;
    }
}


static FeatureId sampleFeatRES(std::mt19937& rng) {
    std::uniform_int_distribution<int> U(0, 4);
    switch (U(rng)) {
    case 0: return FeatureId::RES_WAGE;
    case 1: return FeatureId::RES_SKILL_LEVEL;
    case 2: return FeatureId::RES_FREE_TIME;
    case 3: return FeatureId::RES_MULTI_SKILL;
    default: return FeatureId::RES_UTILIZATION;
    }
}

static int growMS(std::mt19937& rng, std::vector<GPNode>& v, int depth, int maxDepth) {
    std::uniform_real_distribution<double> U01(0.0, 1.0);
    if (depth == maxDepth || U01(rng) < 0.25) {
        if (U01(rng) < 0.7) { GPNode f; f.kind = NodeKind::FEATURE; f.feat = sampleFeatMS(rng); return GPTree::add(v, f); }
        GPNode c; c.kind = NodeKind::CONST; c.constant = (U01(rng) * 2.0 - 1.0); return GPTree::add(v, c);
    }
    GPNode b; b.kind = NodeKind::BINARY; b.bop = sampleB(rng);
    b.left = growMS(rng, v, depth + 1, maxDepth);
    b.right = growMS(rng, v, depth + 1, maxDepth);
    return GPTree::add(v, b);
}

static int growRES(std::mt19937& rng, std::vector<GPNode>& v, int depth, int maxDepth) {
    std::uniform_real_distribution<double> U01(0.0, 1.0);
    if (depth == maxDepth || U01(rng) < 0.25) {
        if (U01(rng) < 0.7) {
            GPNode f;
            f.kind = NodeKind::FEATURE;
            f.feat = sampleFeatRES(rng);
            return GPTree::add(v, f);
        }
        GPNode c;
        c.kind = NodeKind::CONST;
        c.constant = (U01(rng) * 2.0 - 1.0) * 10.0;
        return GPTree::add(v, c);
    }
    GPNode b;
    b.kind = NodeKind::BINARY;
    b.bop = sampleB(rng);
    b.left = growRES(rng, v, depth + 1, maxDepth);
    b.right = growRES(rng, v, depth + 1, maxDepth);
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

GPTree GPTree::Make_EST_plus_DUR() {
    GPTree t;
    GPNode es; es.kind = NodeKind::FEATURE; es.feat = FeatureId::EST_PREC;    int ex = GPTree::add(t.nodes, es);
    GPNode du; du.kind = NodeKind::FEATURE; du.feat = FeatureId::DURATION;    int dx = GPTree::add(t.nodes, du);
    GPNode add; add.kind = NodeKind::BINARY; add.bop = BinaryOp::ADD; add.left = ex; add.right = dx;
    t.root = GPTree::add(t.nodes, add);
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

GPTree GPTree::Make_CHEAP_PER_SKILL_plus_EST() {
    GPTree t;
    GPNode a; a.kind = NodeKind::FEATURE; a.feat = FeatureId::COST_PER_SKILL_NOW;
    int ax = (int)t.nodes.size(); t.nodes.push_back(a);
    GPNode b; b.kind = NodeKind::FEATURE; b.feat = FeatureId::EST_PREC;
    int bx = (int)t.nodes.size(); t.nodes.push_back(b);
    GPNode add; add.kind = NodeKind::BINARY; add.bop = BinaryOp::ADD; add.left = ax; add.right = bx;
    t.root = (int)t.nodes.size(); t.nodes.push_back(add);
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


std::vector<FeatureId> GPTree::allFeatures() {
    return {
        FeatureId::DURATION, FeatureId::REQ_LEVEL, FeatureId::AVAIL_SKILL,
        FeatureId::EST_PREC, FeatureId::SUCC_COUNT, FeatureId::CRITLEN,
        FeatureId::SLACK, FeatureId::AVAIL_GAP, FeatureId::WAIT_RES,
        FeatureId::TOT_PRED, FeatureId::CHEAPEST_COST_NOW, FeatureId::COST_PER_SKILL_NOW,
        FeatureId::MIN_WAGE_AVAIL, FeatureId::AVG_WAGE_AVAIL, FeatureId::TEAM_SIZE_MIN_NOW,
        FeatureId::NUM_TASKS, FeatureId::NUM_RESOURCES, FeatureId::NUM_SKILLS,
        FeatureId::TASK_RES_COUNT, FeatureId::AVG_RES_COST, FeatureId::UNSCHED_TASKS
    };
}

std::vector<UnaryOp> GPTree::allUnaryOps() {
    return { UnaryOp::NEG, UnaryOp::ABS };
}

std::vector<BinaryOp> GPTree::allBinaryOps() {
    return { BinaryOp::ADD, BinaryOp::SUB, BinaryOp::MUL, BinaryOp::DIV, BinaryOp::MIN, BinaryOp::MAX };
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

