#include <cmath>
#include <limits>
#include "GPTreeRule.hpp"

ScoreTrace GPTreeRule::scoreWithTraceFast(int taskIx, const Task& t) const {
    ScoreTrace out;

    if (!m_inst) {
        out.score = 1e30;
        out.feasible = false;
        return out;
    }

    PriorityContext ctx;
    ctx.inst = m_inst;
    ctx.now = m_now;

    Features f = computeFeatures(ctx, taskIx);

    out.feasible = f.feasibleNow;
    out.feat = f;

    if (!f.feasibleNow) {
        out.score = std::numeric_limits<double>::infinity();
        return out;
    }

    double val = m_tree.eval(f);
    if (!std::isfinite(val)) val = 1e30;
    out.score = val;
    return out;
}

ScoreTrace GPTreeRule::scoreWithTrace(const Task& t) const {
    if (!m_inst) {
        ScoreTrace out;
        out.score = 1e30;
        out.feasible = false;
        return out;
    }

    int taskIx = (int)m_inst->idToIndex.at(t.id);
    return scoreWithTraceFast(taskIx, t);
}

double GPTreeRule::scoreFast(int taskIx, const Task& t) const {
    (void)t;

    if (!m_inst) {
        return 1e30;
    }

    PriorityContext ctx;
    ctx.inst = m_inst;
    ctx.now = m_now;

    Features f = computeFeatures(ctx, taskIx);

    if (!f.feasibleNow) {
        return std::numeric_limits<double>::infinity();
    }

    double val = m_tree.eval(f);
    if (!std::isfinite(val)) {
        val = 1e30;
    }

    return val;
}

double GPTreeRule::score(const Task& t) const {
    return scoreWithTrace(t).score;
}