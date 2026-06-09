#pragma once
#include <string>
#include <vector>
#include <random>
#include <sstream>
#include <algorithm>
#include "Op.hpp"
#include "Features.hpp"


enum class FeatureId {
    DURATION,
    REQ_LEVEL,
    AVAIL_SKILL,
    CRITLEN,
    SLACK,
    DESC_COUNT,
    TASK_RELEASE_PRESSURE,
    TASK_CRITICAL_PRESSURE,
    AVAIL_GAP,
    CHEAPEST_COST_NOW,
    COST_PER_SKILL_NOW,
    TASK_RES_COUNT,
    AVG_RES_COST,
    UNSCHED_TASKS,

    MIN_FEASIBLE_COST_NOW,
    COST_REGRET_NOW,

    RES_WAGE,
    RES_SKILL_LEVEL,
    RES_IDLE_TIME,
    RES_CAN_START_NOW,
    RES_UTILIZATION,
    RES_WAGE_PER_LEVEL,
    RES_ASSIGN_COST,
    RES_ASSIGN_PREMIUM_ALL,
    RES_RESERVE_PRESSURE,
    RES_FAMILY_MISMATCH,
    RES_FUTURE_BRANCH_FIT,
    RES_BOTTLENECK_PRESERVATION,
    RES_SPECIALIST_MISUSE,
    RES_RELATIVE_WAGE
};

enum class NodeKind { CONST, FEATURE, UNARY, BINARY };
enum class UnaryOp { NEG, ABS };
enum class BinaryOp { ADD, SUB, MUL, DIV, MIN, MAX };

struct GPNode {
    NodeKind kind{};
    double constant = 0.0;
    FeatureId feat{};
    UnaryOp  uop{};
    BinaryOp bop{};
    int left = -1;
    int right = -1;
};

class GPTree {
public:
    int root = -1;
    std::vector<GPNode> nodes;

    bool isEmpty() const { return root < 0 || nodes.empty(); }
    bool validIndex(int nodeId) const { return nodeId >= 0 && nodeId < (int)nodes.size(); }

    double      eval(const Features& f) const;
    double      evalWithNodeValues(const Features& f, std::vector<double>* nodeValues) const;
    std::string toString() const;

    bool hasAnyFeature() const {
        for (const auto& n : nodes) if (n.kind == NodeKind::FEATURE) return true;
        return false;
    }

    static GPTree RandomTreeMS(std::mt19937& rng, int maxDepth);
    static GPTree RandomTreeRES(std::mt19937& rng, int maxDepth);
    static GPTree RandomTreePAIR(std::mt19937& rng, int maxDepth);
    static GPTree Make_AVAIL_minus_REQ();
    static GPTree Make_REQ_times_DUR();
    static GPTree Make_CHEAPxDUR();
    static GPTree Make_CHEAP_PER_SKILL_plus_DUR();
    static GPTree Make_BNTGA_BridgeRatioPair();
    static int add(std::vector<GPNode>& v, const GPNode& n) {
        v.push_back(n); return (int)v.size() - 1;
    }


    int nodeCount() const { return (int)nodes.size(); }
    int depth() const;
    int nodeDepth(int nodeId) const;

    int subtreeHeight(int index) const;

    static std::vector<FeatureId> allTaskFeatures();
    static std::vector<FeatureId> allResFeatures();
    static std::vector<FeatureId> allPairFeatures();
    static std::vector<FeatureId> allFeatures() { return allPairFeatures(); }

    GPTree extractSubtree(int nodeId) const;
    void   replaceSubtree(int nodeId, const GPTree& sub);

    GPTree graftedWith(int replaceIndex, const GPTree& donor) const;

    bool isStructurallySound() const;

private:
    double      featureValue(FeatureId id, const Features& f) const;
    double      evalAt(int idx, const Features& f) const;
    double      evalAtCollect(int idx, const Features& f, std::vector<double>& vals) const;
    std::string toStringAt(int idx) const;

    std::string nodeLabel(int idx) const;
    void buildAscii(int idx, const std::string& indent, bool last, bool unicode, std::string& out) const;

    int depthAt(int nodeId) const;
    int cloneSubtreeDFS(int nodeId, std::vector<int>& order) const;
    int rebuildWithReplace(int nodeId,
        const GPTree* sub,
        int replaceAt,
        std::vector<GPNode>& out) const;
};
