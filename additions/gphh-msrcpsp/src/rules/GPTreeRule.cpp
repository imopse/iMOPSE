#include <cmath>
#include <limits>
#include "rules/GPTreeRule.hpp"
#include "gp/Features.hpp"


ScoreTrace GPTreeRule::scoreWithTrace(const Task& t) const {
    ScoreTrace out;

    if (!m_inst) {
        out.score = 1e30;
        out.feasible = false;
        return out;
    }

    PriorityContext ctx;
    ctx.inst = m_inst;
    ctx.now = m_now;

    int taskIx = (int)m_inst->idToIndex.at(t.id);
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

double GPTreeRule::score(const Task& t) const {
    return scoreWithTrace(t).score;
}
