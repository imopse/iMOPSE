#pragma once
#include <string>
#include <limits>
#include "IDispatchingRule.hpp"
#include "../gp/GPTree.hpp"
#include "../gp/Features.hpp"
#include "../domain/Instance.hpp"

struct ScoreTrace {
    double   score = std::numeric_limits<double>::infinity();
    Features feat{};
    bool     feasible = false;
};

class GPTreeRule : public IDispatchingRule {
public:
    explicit GPTreeRule(const GPTree& t) : m_tree(t) {}

    double score(const Task& t) const override;               
    std::string name() const override { return m_tree.toString(); }
    void setContext(const Instance* inst, int now) override {  
        m_inst = inst; m_now = now;
    }

    ScoreTrace scoreWithTrace(const Task& t) const;

    const GPTree& getTree() const { return m_tree; }
    std::string   exprString() const { return m_tree.toString(); }

private:
    const Instance* m_inst = nullptr;
    int             m_now = 0;
    GPTree          m_tree;

};
