#pragma once
#include <string>
#include "IDispatchingRule.hpp"
#include "../gp/GPTree.hpp"
#include "../gp/Features.hpp"
#include "../domain/Instance.hpp"

class GPTreeRule : public IDispatchingRule {
public:
    explicit GPTreeRule(const GPTree& t) : m_tree(t) {}

    double score(const Task& t) const override;
    double scoreFast(int taskIx, const Task& t) const;

    std::string name() const override { return m_tree.toString(); }

    void setContext(const Instance* inst, int now) override {
        m_inst = inst;
        m_now = now;
    }

    const GPTree& getTree() const { return m_tree; }

private:
    const Instance* m_inst = nullptr;
    int             m_now = 0;
    GPTree          m_tree;
};