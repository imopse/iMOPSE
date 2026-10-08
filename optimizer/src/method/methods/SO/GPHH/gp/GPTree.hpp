#pragma once
#include <string>
#include <vector>
#include <random>
#include <sstream>
#include <algorithm>
#include "Op.hpp"
#include "Features.hpp"

namespace gphh_so {

    enum class FeatureId {
        TASK_UNLOCK_BUCKET,
        TASK_SCARCITY_BUCKET,
        TASK_LONG_FLAG,
        TASK_READY_AGE,
        RES_COST_PREMIUM,
        RES_SCARCE_FAMILY_LOAD,
        RES_WAIT_IF_CHOSEN,
        PAIR_SPECIALIST_MISUSE
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
    std::string toJson() const;
    std::string debugNodeLabel(int idx) const { return nodeLabel(idx); }

    bool hasAnyFeature() const {
        for (const auto& n : nodes) if (n.kind == NodeKind::FEATURE) return true;
        return false;
    }

    static GPTree RandomTreeMS(std::mt19937& rng, int maxDepth);
    static GPTree RandomTreeRES(std::mt19937& rng, int maxDepth);
    static GPTree RandomTreePAIR(std::mt19937& rng, int maxDepth);
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

}                     