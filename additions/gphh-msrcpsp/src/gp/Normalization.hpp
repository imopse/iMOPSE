#pragma once
#include "Instance.hpp"
#include "Precompute.hpp"   // CPMPrecalc + buildCPM (namespace gp)

// Sta³e granice min/max dla 2 celów: makespan i cost.
struct ImopseBounds {
    int    ms_min;   // minimalny makespan
    int    ms_max;   // maksymalny makespan
    double cost_min; // minimalny koszt
    double cost_max; // maksymalny koszt
};

// Wyliczenie granic min/max na podstawie instancji (CPM + min/max stawek)
ImopseBounds compute_imopse_bounds(const Instance& I);

// Normalizacja min–max do [0,1] dla (ms, cost)
inline std::pair<double, double> imopse_minmax_normalize(int ms, double cost, const ImopseBounds& b) {
    auto safe01 = [](double x, double lo, double hi) {
        const double eps = 1e-9;
        if (hi <= lo + eps) return 0.0;
        double v = (x - lo) / (hi - lo);
        if (v < 0.0) v = 0.0;
        if (v > 1.0) v = 1.0;
        return v;
        };
    return {
        safe01(double(ms), double(b.ms_min), double(b.ms_max)),
        safe01(cost,       b.cost_min,       b.cost_max)
    };
}
