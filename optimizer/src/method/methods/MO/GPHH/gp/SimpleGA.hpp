#pragma once
#include <vector>
#include <random>
#include <algorithm>
#include <limits>
#include <utility>
#include <cstdint>
#include <cmath>
#include <string>
#include "../scheduler/Scheduler.hpp"
#include "../rules/GPTreeRule.hpp"
#include "../gp/GPTree.hpp"
#include "../gp/Precompute.hpp"
#include "../gp/Normalization.hpp"



struct GA_Params {
    size_t popSize = 100;
    size_t generations = 1000;
    double pCrossover = 0.9;
    double pMutation = 0.1;
    uint64_t seed = 1234567;
    double weight = 0.5;
};

struct GA_Individual {
    std::vector<float> keys;
    double fitness = std::numeric_limits<double>::infinity();
    int makespan = 0;
    double cost = 0.0;
    double msNorm = 0.0;
    double costNorm = 0.0;
};

static inline double scalar_fitness_from_norm(double msNorm, double costNorm, double w) {
    return w * msNorm + (1.0 - w) * costNorm;
}

class SimpleGA {
public:
    SimpleGA(Instance& I,
        const GPTree& ruleTree,
        const GA_Params& P)
        : inst(I), gpRule(ruleTree), params(P), rng((unsigned)P.seed) {
        gp::buildCPM(inst, cpm);
        gp::setCPMPrecalc(&cpm);
        bounds = compute_imopse_bounds(inst);
        nTasks = (int)inst.tasks.size();
        gpRuleWrap = std::make_unique<GPTreeRule>(gpRule);
    }

    GA_Individual run() {
        initPopulation();
        evalPopulation();

        GA_Individual best = *std::min_element(pop.begin(), pop.end(),
            [](auto& a, auto& b) { return a.fitness < b.fitness; });

        for (size_t gen = 0; gen < params.generations; ++gen) {
            std::vector<GA_Individual> next;
            next.reserve(pop.size());

            next.push_back(best);

            while (next.size() < pop.size()) {
                const GA_Individual& p1 = tournament(3);
                const GA_Individual& p2 = tournament(3);
                GA_Individual c1 = p1, c2 = p2;

                if (rand01() < params.pCrossover) {
                    blendCrossover(c1, c2);
                }
                mutate(c1);
                mutate(c2);

                next.push_back(std::move(c1));
                if (next.size() < pop.size())
                    next.push_back(std::move(c2));
            }

            pop.swap(next);
            evalPopulation();

            const GA_Individual genBest = *std::min_element(pop.begin(), pop.end(),
                [](auto& a, auto& b) { return a.fitness < b.fitness; });
            if (genBest.fitness < best.fitness) best = genBest;
        }
        return best;
    }

private:
    Instance& inst;
    const GPTree& gpRule;
    GA_Params params;
    std::mt19937 rng;
    int nTasks = 0;

    gp::CPMPrecalc cpm{};
    ImopseBounds bounds{};
    std::unique_ptr<GPTreeRule> gpRuleWrap;

    std::vector<GA_Individual> pop;

private:
    double rand01() {
        return std::uniform_real_distribution<double>(0.0, 1.0)(rng);
    }
    float randKey() {
        return std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
    }

    void initPopulation() {
        pop.clear();
        pop.resize(params.popSize);

        for (size_t i = 0; i < pop.size(); ++i) {
            pop[i].keys.resize(nTasks);
            for (int k = 0; k < nTasks; ++k) pop[i].keys[k] = randKey();
        }

        seed_cheap_first(pop[0].keys);
        if (pop.size() > 1) seed_balanced_usage(pop[1].keys);
    }

    void seed_cheap_first(std::vector<float>& keys) {
        std::vector<std::pair<double, int>> approx; approx.reserve(nTasks);
        for (int i = 0; i < nTasks; ++i) {
            const auto& t = inst.tasks[i];
            double best = std::numeric_limits<double>::infinity();
            for (const auto& r : inst.resources) {
                auto it = r.skills.find(t.reqSkill);
                if (it == r.skills.end()) continue;
                if (t.reqLevel <= 0 || it->second > 0) {
                    best = std::min(best, r.salary);
                }
            }
            if (!std::isfinite(best)) best = 1e6;
            approx.emplace_back(best * t.duration, i);
        }
        std::sort(approx.begin(), approx.end());
        keys.assign(nTasks, 0.0f);
        for (int rank = 0; rank < nTasks; ++rank) {
            keys[approx[rank].second] = float(rank) / float(std::max(1, nTasks - 1));
        }
    }

    void seed_balanced_usage(std::vector<float>& keys) {
        keys.resize(nTasks);
        for (int i = 0; i < nTasks; ++i) {
            const auto& t = inst.tasks[i];
            int cap = 0;
            for (const auto& r : inst.resources) {
                auto it = r.skills.find(t.reqSkill);
                if (it == r.skills.end()) continue;
                if (t.reqLevel <= 0 || it->second > 0) cap++;
            }
            float base = randKey();
            float bonus = (cap > 0) ? (0.5f / float(cap)) : 0.5f;
            keys[i] = std::clamp(base + bonus, 0.0f, 1.0f);
        }
    }

    void blendCrossover(GA_Individual& a, GA_Individual& b) {
        const float alpha = 0.1f;
        for (int i = 0; i < nTasks; ++i) {
            float x = a.keys[i], y = b.keys[i];
            float lo = std::min(x, y), hi = std::max(x, y);
            float span = hi - lo;
            float L = lo - alpha * span;
            float H = hi + alpha * span;
            std::uniform_real_distribution<float> dist(L, H);
            a.keys[i] = std::clamp(dist(rng), 0.0f, 1.0f);
            b.keys[i] = std::clamp(dist(rng), 0.0f, 1.0f);
        }
    }

    void mutate(GA_Individual& ind) {
        for (int i = 0; i < nTasks; ++i) {
            if (rand01() < params.pMutation) {
                float v = ind.keys[i] + std::normal_distribution<float>(0.0f, 0.1f)(rng);
                if (rand01() < 0.2) v = randKey();
                ind.keys[i] = std::clamp(v, 0.0f, 1.0f);
            }
        }
    }

    const GA_Individual& tournament(int k) {
        int best = -1;
        for (int i = 0; i < k; ++i) {
            int j = std::uniform_int_distribution<int>(0, int(pop.size()) - 1)(rng);
            if (best == -1 || pop[j].fitness < pop[best].fitness) best = j;
        }
        return pop[best];
    }

    void evalPopulation() {
        for (auto& ind : pop) {
            Scheduler::setPriorityKeys(&ind.keys);

            Instance Icopy = inst;
            auto res = Scheduler::withResources(Icopy, *gpRuleWrap);


            ind.makespan = res.makespan;
            ind.cost = res.totalCost;

            auto [msN, cN] = imopse_minmax_normalize(ind.makespan, ind.cost, bounds);
            ind.msNorm = msN;
            ind.costNorm = cN;
            ind.fitness = scalar_fitness_from_norm(ind.msNorm, ind.costNorm, params.weight);
        }

        Scheduler::setPriorityKeys(nullptr);
    }
};
