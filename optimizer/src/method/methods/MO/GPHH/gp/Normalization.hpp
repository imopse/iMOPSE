#pragma once
#include "../domain/Instance.hpp"
#include "Precompute.hpp"


struct ImopseBounds {
    int    ms_min;
    int    ms_max;
    double cost_min;
    double cost_max;
};


ImopseBounds compute_imopse_bounds(const Instance& I);


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
