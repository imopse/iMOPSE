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

namespace gphh_so {

double GPTree::featureValue(FeatureId id, const Features& f) const {
    switch (id) {
    case FeatureId::TASK_UNLOCK_BUCKET:     return f.taskUnlockBucket;
    case FeatureId::TASK_SCARCITY_BUCKET:   return f.taskScarcityBucket;
    case FeatureId::TASK_LONG_FLAG:         return f.taskLongFlag;
    case FeatureId::TASK_READY_AGE:         return f.taskReadyAge;
    case FeatureId::RES_COST_PREMIUM:       return f.resCostPremium;
    case FeatureId::RES_SCARCE_FAMILY_LOAD: return f.resScarceFamilyLoad;
    case FeatureId::RES_WAIT_IF_CHOSEN:     return f.resWaitIfChosen;
    case FeatureId::PAIR_SPECIALIST_MISUSE: return f.pairSpecialistMisuse;
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
        return oss.str();

    case NodeKind::FEATURE: {
        const char* nm = "?";
        switch (n.feat) {
        case FeatureId::TASK_UNLOCK_BUCKET:    nm = "TASK_DOWNSTREAM_WORK"; break;
        case FeatureId::TASK_SCARCITY_BUCKET:  nm = "TASK_SCARCITY"; break;
        case FeatureId::TASK_LONG_FLAG:        nm = "TASK_ROOT_EARLY_GAIN"; break;
        case FeatureId::TASK_READY_AGE:        nm = "TASK_SLACK_PRESSURE"; break;
        case FeatureId::RES_COST_PREMIUM:      nm = "RES_DOMINATED_CHOICE"; break;
        case FeatureId::RES_SCARCE_FAMILY_LOAD: nm = "RES_SCARCE_FAMILY_LOAD"; break;
        case FeatureId::RES_WAIT_IF_CHOSEN:     nm = "RES_WAIT_IF_CHOSEN"; break;
        case FeatureId::PAIR_SPECIALIST_MISUSE: nm = "PAIR_SPECIALIST_MISUSE"; break;
        }
        return nm;
    }

    case NodeKind::UNARY:
        oss << (n.uop == UnaryOp::NEG ? "NEG" : "ABS");
        return oss.str();

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
        return oss.str();
    }
    }

    return "<?>";
}

std::string GPTree::toString() const {
    if (isEmpty()) return "CONST(0)";
    return toStringAt(root);
}

namespace {
    static const char* featToString(FeatureId f) {
        switch (f) {
        case FeatureId::TASK_UNLOCK_BUCKET:    return "TASK_DOWNSTREAM_WORK";
        case FeatureId::TASK_SCARCITY_BUCKET:  return "TASK_SCARCITY";
        case FeatureId::TASK_LONG_FLAG:        return "TASK_ROOT_EARLY_GAIN";
        case FeatureId::TASK_READY_AGE:        return "TASK_SLACK_PRESSURE";
        case FeatureId::RES_COST_PREMIUM:      return "RES_DOMINATED_CHOICE";
        case FeatureId::RES_SCARCE_FAMILY_LOAD: return "RES_SCARCE_FAMILY_LOAD";
        case FeatureId::RES_WAIT_IF_CHOSEN:     return "RES_WAIT_IF_CHOSEN";
        case FeatureId::PAIR_SPECIALIST_MISUSE: return "PAIR_SPECIALIST_MISUSE";
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
        FeatureId::TASK_UNLOCK_BUCKET,
        FeatureId::TASK_SCARCITY_BUCKET,
        FeatureId::TASK_LONG_FLAG,
        FeatureId::TASK_READY_AGE
    };
}

std::vector<FeatureId> GPTree::allResFeatures() {
    return {
        FeatureId::RES_COST_PREMIUM,
        FeatureId::RES_SCARCE_FAMILY_LOAD,
        FeatureId::RES_WAIT_IF_CHOSEN,
        FeatureId::PAIR_SPECIALIST_MISUSE
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

}