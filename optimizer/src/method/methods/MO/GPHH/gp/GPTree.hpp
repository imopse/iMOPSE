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
    EST_PREC,
    SUCC_COUNT,
    CRITLEN,
    SLACK,
    AVAIL_GAP,
    WAIT_RES,
    TOT_PRED,
    CHEAPEST_COST_NOW,
    COST_PER_SKILL_NOW,
    MIN_WAGE_AVAIL,
    AVG_WAGE_AVAIL,
    TEAM_SIZE_MIN_NOW,
    NUM_TASKS,
    NUM_RESOURCES,
    NUM_SKILLS,
    TASK_RES_COUNT,
    AVG_RES_COST,
    UNSCHED_TASKS,
    RES_WAGE,
    RES_SKILL_LEVEL,
    RES_FREE_TIME,
    RES_MULTI_SKILL,
    RES_UTILIZATION
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
    std::string toString() const;
    std::string toJson() const;

    bool hasAnyFeature() const {
        for (const auto& n : nodes) if (n.kind == NodeKind::FEATURE) return true;
        return false;
    }

    static GPTree RandomTreeMS(std::mt19937& rng, int maxDepth);
    static GPTree RandomTreeRES(std::mt19937& rng, int maxDepth);
    static GPTree Make_AVAIL_minus_REQ();
    static GPTree Make_REQ_times_DUR();
    static GPTree Make_EST_plus_DUR();
    static GPTree Make_CHEAPxDUR();
    static GPTree Make_CHEAP_PER_SKILL_plus_EST();
    static int add(std::vector<GPNode>& v, const GPNode& n) {
        v.push_back(n); return (int)v.size() - 1;
    }


    int nodeCount() const { return (int)nodes.size(); }
    int depth() const;
    int nodeDepth(int nodeId) const;

    int subtreeHeight(int index) const;

    bool isConst(int nodeId)   const { return validIndex(nodeId) && nodes[nodeId].kind == NodeKind::CONST; }
    bool isFeature(int nodeId) const { return validIndex(nodeId) && nodes[nodeId].kind == NodeKind::FEATURE; }
    bool isUnary(int nodeId)   const { return validIndex(nodeId) && nodes[nodeId].kind == NodeKind::UNARY; }
    bool isBinary(int nodeId)  const { return validIndex(nodeId) && nodes[nodeId].kind == NodeKind::BINARY; }

    FeatureId featureOf(int nodeId) const {
        return validIndex(nodeId) ? nodes[nodeId].feat : FeatureId::DURATION;
    }
    void setFeature(int nodeId, FeatureId f) {
        if (!validIndex(nodeId)) return;
        nodes[nodeId].kind = NodeKind::FEATURE; nodes[nodeId].feat = f;
    }

    UnaryOp unaryOp(int nodeId) const {
        return validIndex(nodeId) ? nodes[nodeId].uop : UnaryOp::ABS;
    }
    void setUnaryOp(int nodeId, UnaryOp u) {
        if (!validIndex(nodeId)) return;
        nodes[nodeId].kind = NodeKind::UNARY; nodes[nodeId].uop = u;
    }

    BinaryOp binaryOp(int nodeId) const {
        return validIndex(nodeId) ? nodes[nodeId].bop : BinaryOp::ADD;
    }
    void setBinaryOp(int nodeId, BinaryOp b) {
        if (!validIndex(nodeId)) return;
        nodes[nodeId].kind = NodeKind::BINARY; nodes[nodeId].bop = b;
    }

    void setConst(int nodeId, double v) {
        if (!validIndex(nodeId)) return;
        nodes[nodeId].kind = NodeKind::CONST; nodes[nodeId].constant = v;
    }

    static std::vector<FeatureId> allFeatures();
    static std::vector<UnaryOp>   allUnaryOps();
    static std::vector<BinaryOp>  allBinaryOps();

    GPTree extractSubtree(int nodeId) const;
    void   replaceSubtree(int nodeId, const GPTree& sub);

    GPTree graftedWith(int replaceIndex, const GPTree& donor) const;

    bool isStructurallySound() const;

private:
    double      featureValue(FeatureId id, const Features& f) const;
    double      evalAt(int idx, const Features& f) const;
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
