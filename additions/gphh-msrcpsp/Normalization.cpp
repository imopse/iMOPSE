#include "gp/Normalization.hpp"
#include <limits>
#include <algorithm>
#include <queue>

static int critical_path_length(const Instance& I)
{
    const int n = (int)I.tasks.size();
    std::vector<std::vector<int>> adj(n);
    std::vector<int> indeg(n, 0);

    for (int j = 0; j < n; ++j) {
        for (int pid : I.tasks[j].predecessors) {
            auto it = I.idToIndex.find(pid);
            if (it != I.idToIndex.end()) {
                int p = it->second;
                adj[p].push_back(j);
                indeg[j]++;
            }
        }
    }

    std::queue<int> q;
    std::vector<int> longest(n, 0);

    for (int i = 0; i < n; ++i)
        if (indeg[i] == 0) q.push(i);

    int bestFinish = 0;
    while (!q.empty()) {
        int u = q.front(); q.pop();
        int finishU = longest[u] + I.tasks[u].duration;
        bestFinish = std::max(bestFinish, finishU);

        for (int v : adj[u]) {
            longest[v] = std::max(longest[v], finishU);
            if (--indeg[v] == 0) q.push(v);
        }
    }
    return bestFinish;
}

static double estimate_min_cost(const Instance& I)
{
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

static double estimate_max_cost(const Instance& I)
{
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

ImopseBounds compute_imopse_bounds(const Instance& I)
{
    int ms_min = critical_path_length(I);

    int ms_max = 0;
    for (const auto& t : I.tasks) ms_max += t.duration;
    if (ms_max <= ms_min) ms_max = ms_min + 1;

    double cost_min = estimate_min_cost(I);
    double cost_max = estimate_max_cost(I);
    if (cost_max <= cost_min) cost_max = cost_min + 1.0;

    return { ms_min, ms_max, cost_min, cost_max };
}
