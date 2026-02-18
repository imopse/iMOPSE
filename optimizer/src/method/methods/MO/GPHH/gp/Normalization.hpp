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


inline std::pair<double, double> imopse_minmax_normalize(int ms, double cost, const ImopseBounds& b)
{
    const double dMs = double(b.ms_max - b.ms_min);
    const double dCost = (b.cost_max - b.cost_min);

    const double msN = (dMs != 0.0) ? ((double(ms) - double(b.ms_min)) / dMs) : 0.0;
    const double costN = (dCost != 0.0) ? ((cost - b.cost_min) / dCost) : 0.0;

    return { msN, costN };
}
