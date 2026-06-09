#include <cmath>
#include <limits>
#include "GPTreeRule.hpp"

double GPTreeRule::scoreFast(int taskIx, const Task& t) const {
    (void)t;

    if (!m_inst) {
        return 1e30;
    }

    PriorityContext ctx;
    ctx.inst = m_inst;
    ctx.now = m_now;

    Features f = computeFeaturesFast(ctx, taskIx);

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
    if (!m_inst) {
        return 1e30;
    }

    int taskIx = (int)m_inst->idToIndex.at(t.id);
    return scoreFast(taskIx, t);
}