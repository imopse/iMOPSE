#include "Normalization.hpp"
#include <limits>
#include <algorithm>

// MIN koszt: dla każdego zadania najtańszy zasób z wymaganym skillem.
// Twoja semantyka: reqLevel==0 → można przydzielić zasób z level 0;
// reqLevel>=1 → zasób musi mieć level > 0.
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

// MAX koszt: dla każdego zadania najdroższy zasób z wymaganym skillem (jw.).
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
    // MIN makespan = długość ścieżki krytycznej (CPM – bez ograniczeń zasobów)
    gp::CPMPrecalc local;
    gp::buildCPM(I, local);
    int ms_min = std::max(0, local.cpathLen);

    // MAX makespan = pesymistyczna sekwencja (suma czasów wszystkich zadań)
    int ms_max = 0;
    for (const auto& t : I.tasks) ms_max += t.duration;
    ms_max = std::max(ms_max, ms_min + 1); // uniknij zakresu 0

    // MIN / MAX cost (sumy po zadaniach)
    double cost_min = estimate_min_cost(I);
    double cost_max = estimate_max_cost(I);
    if (cost_max <= cost_min) cost_max = cost_min + 1.0; // uniknij dzielenia przez 0

    return { ms_min, ms_max, cost_min, cost_max };
}
