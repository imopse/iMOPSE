#pragma once
#include <vector>
#include <algorithm>
#include <utility>
#include <cmath>

struct SMOIndividual;

namespace Metrics {

    inline double clamp01(double v) {
        if (!std::isfinite(v)) return 1.0;
        if (v < 0.0) return 0.0;
        if (v > 1.0) return 1.0;
        return v;
    }

    inline double HV2D_Ref11_FromArchive(const std::vector<SMOIndividual*>& arch) {
        if (arch.empty()) return 0.0;

        std::vector<std::pair<double, double>> pts;
        pts.reserve(arch.size());
        for (auto* ind : arch) {
            double x = clamp01(ind->m_NormalizedEvaluation[0]);
            double y = clamp01(ind->m_NormalizedEvaluation[1]);
            pts.emplace_back(x, y);
        }

        std::sort(pts.begin(), pts.end(), [](auto& a, auto& b) {
            if (a.first != b.first) return a.first < b.first;
            return a.second < b.second;
            });

        double hv = 0.0;
        double prevX = 1.0;
        double bestY = 1.0;

        for (int i = (int)pts.size() - 1; i >= 0; --i) {
            const double x = pts[i].first;
            const double y = pts[i].second;
            if (y < bestY) bestY = y;

            const double w = prevX - x;
            const double h = 1.0 - bestY;
            if (w > 0.0 && h > 0.0) hv += w * h;

            prevX = x;
        }
        return hv;
    }

} // namespace Metrics