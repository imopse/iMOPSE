#include "Normalization.hpp"
#include <limits>
#include <algorithm>
#include <cmath>

ImopseBounds compute_imopse_bounds(const Instance& I)
{
    int sumDur = 0;
    int minDur = std::numeric_limits<int>::max();

    for (const auto& t : I.tasks) {
        sumDur += t.duration;
        minDur = std::min(minDur, t.duration);
    }
    if (I.tasks.empty()) {
        sumDur = 0;
        minDur = 0;
    }

    int ms_min = 0;
    const int taskCount = (int)I.tasks.size();
    const int resCount = (int)I.resources.size();
    if (taskCount > 0 && resCount > 0) {
        long long tmp = 1LL * minDur * taskCount;
        ms_min = (int)(tmp / resCount);
    }

    int ms_max = sumDur;

    double minSalary = std::numeric_limits<double>::infinity();
    double maxSalary = 0.0;

    for (const auto& r : I.resources) {
        minSalary = std::min(minSalary, (double)r.salary);
        maxSalary = std::max(maxSalary, (double)r.salary);
    }
    if (!std::isfinite(minSalary)) minSalary = 0.0;

    double cost_min = (double)sumDur * minSalary;
    double cost_max = (double)sumDur * maxSalary;

    if (ms_max <= ms_min) ms_max = ms_min + 1;
    if (cost_max <= cost_min) cost_max = cost_min + 1.0;

    return { ms_min, ms_max, cost_min, cost_max };
}
