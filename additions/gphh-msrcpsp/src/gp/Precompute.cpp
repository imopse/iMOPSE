#include "gp/Precompute.hpp"
#include <queue>
#include <algorithm>
#include <unordered_set>

namespace {
    const gp::CPMPrecalc* g_cpm_ptr = nullptr;
}

namespace gp {

    static std::vector<int> topoOrder(const Instance& I) {
        std::vector<int> indeg(I.tasks.size(), 0);
        for (size_t v = 0; v < I.tasks.size(); ++v)
            for (int p : I.tasks[v].predecessors)
                indeg[v]++;

        std::queue<int> q;
        for (size_t i = 0;i < indeg.size();++i) if (!indeg[i]) q.push((int)i);
        std::vector<int> order;
        while (!q.empty()) {
            int v = q.front(); q.pop(); order.push_back(v);
            int idv = I.tasks[v].id;
            for (size_t u = 0; u < I.tasks.size(); ++u) {
                for (int p : I.tasks[u].predecessors)
                    if (p == idv) { if (--indeg[u] == 0) q.push((int)u); }
            }
        }
        return order;
    }

    bool buildCPM(const Instance& I, CPMPrecalc& out)
    {
        const int N = (int)I.tasks.size();
        out.est.assign(N, 0);
        std::vector<int> eft(N, 0);
        out.lst.assign(N, 0);
        out.critLen.assign(N, 0);
        out.totPred.assign(N, 0);
        out.cmaxCPM = 0;
        out.slack.assign(N, 0);
        out.succCount.assign(N, 0);

        auto topo = topoOrder(I);
        if ((int)topo.size() != N) return false;

        for (int v : topo) {
            int estv = 0;
            for (int pid : I.tasks[v].predecessors) {
                int p = I.idToIndex.at(pid);
                estv = std::max(estv, eft[p]);
            }
            out.est[v] = estv;
            eft[v] = estv + I.tasks[v].duration;
            out.cmaxCPM = std::max(out.cmaxCPM, eft[v]);
        }

        std::vector<std::vector<int>> succ(N);
        for (size_t u = 0; u < I.tasks.size(); ++u)
            for (int pid : I.tasks[u].predecessors)
                succ[I.idToIndex.at(pid)].push_back((int)u);

        for (int i = N - 1;i >= 0;--i) {
            int v = topo[i];
            int lstv;
            if (succ[v].empty()) {
                lstv = out.cmaxCPM - I.tasks[v].duration;
            }
            else {
                lstv = INT_MAX / 4;
                for (int s : succ[v])
                    lstv = std::min(lstv, out.lst[s] - I.tasks[v].duration);
            }
            out.lst[v] = lstv;
        }

        for (int i = N - 1;i >= 0;--i) {
            int v = topo[i];
            int best = 0;
            for (int s : succ[v])
                best = std::max(best, out.critLen[s]);
            out.critLen[v] = I.tasks[v].duration + best;
        }

        for (int v = 0; v < N; ++v) {
            out.slack[v] = out.lst[v] - out.est[v];
            out.succCount[v] = (int)succ[v].size();
        }

        for (int v = 0; v < N; ++v) {
            std::unordered_set<int> seen;
            std::vector<int> stack;
            for (int pid : I.tasks[v].predecessors)
                stack.push_back(I.idToIndex.at(pid));
            while (!stack.empty()) {
                int u = stack.back(); stack.pop_back();
                if (seen.insert(u).second) {
                    for (int pid : I.tasks[u].predecessors)
                        stack.push_back(I.idToIndex.at(pid));
                }
            }
            out.totPred[v] = (int)seen.size();
        }
        return true;
    }

    void setCPMPrecalc(const CPMPrecalc* ptr) {
        g_cpm_ptr = ptr;
    }

    const CPMPrecalc* getCPMPrecalc() {
        return g_cpm_ptr;
    }

} // namespace gp
