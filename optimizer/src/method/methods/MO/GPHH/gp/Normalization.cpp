#include "Normalization.hpp"
#include <limits>
#include <algorithm>
#include <cmath>

static double estimate_min_cost(const Instance& I) {
    double sum = 0.0;
    for (const auto& t : I.tasks) {
        double best = std::numeric_limits<double>::infinity();
        for (const auto& r : I.resources) {
            auto it = r.skills.find(t.reqSkill);
            if (it == r.skills.end()) continue;
            if (t.reqLevel <= 0 || it->second > 0) {
                best = std::min(best, r.salary);
            }
        }
        if (std::isfinite(best)) sum += best * double(t.duration);
    }
    return sum;
}


static double estimate_max_cost(const Instance& I) {
    double sum = 0.0;
    for (const auto& t : I.tasks) {
        double worst = 0.0;
        bool any = false;
        for (const auto& r : I.resources) {
            auto it = r.skills.find(t.reqSkill);
            if (it == r.skills.end()) continue;
            if (t.reqLevel <= 0 || it->second > 0) {
                worst = std::max(worst, r.salary);
                any = true;
            }
        }
        if (any) sum += worst * double(t.duration);
    }
    return sum;
}

ImopseBounds compute_imopse_bounds(const Instance& I) {
    gp::CPMPrecalc local;
    gp::buildCPM(I, local);
    int ms_min = std::max(0, local.cmaxCPM);

    int ms_max = 0;
    for (const auto& t : I.tasks) ms_max += t.duration;
    ms_max = std::max(ms_max, ms_min + 1);

    double cost_min = estimate_min_cost(I);
    double cost_max = estimate_max_cost(I);
    if (cost_max <= cost_min) cost_max = cost_min + 1.0;

    return { ms_min, ms_max, cost_min, cost_max };
}
