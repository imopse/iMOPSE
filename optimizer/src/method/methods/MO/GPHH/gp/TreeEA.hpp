#pragma once
#include <vector>
#include <random>
#include <limits>
#include <memory>
#include <utility>
#include <algorithm>

#include "../domain/Instance.hpp"
#include "../scheduler/Scheduler.hpp"
#include "../rules/GPTreeRule.hpp"
#include "GPTree.hpp"
#include "Precompute.hpp"
#include "Normalization.hpp"

struct GPEA_Params {
    size_t   popSize = 40;
    size_t   generations = 100;
    double   pCrossover = 0.9;
    double   pMutParam = 0.1;
    double   pMutStruct = 0.05;
    int      maxDepth = 4;
    uint64_t seed = 1234567;
    double   weight = 0.5;
    bool     useNormalization = true;
    int    tournamentK = 3;
    size_t eliteCount = 1;
    bool useNSGA2 = false;
};

struct GP_Individual {
    GPTree taskTree;
    GPTree resTree;
    double fitness = std::numeric_limits<double>::infinity();
    int    makespan = 0;
    double cost = 0.0;
    double msNorm = 0.0;
    double costNorm = 0.0;
    int    rank = 0;
    double crowding = 0.0;
};

struct GP_ParetoPoint {
    int    makespan = 0;
    double cost = 0.0;
    double msNorm = 0.0;
    double costNorm = 0.0;
};

class TreeEA {
public:
    TreeEA(Instance& I, const GPEA_Params& P);
    ~TreeEA();
    void setSeedTrees(const GPTree& task, const GPTree& res);

    GP_Individual run();

    const std::vector<double>& getHistHV() const { return histHV_; }
    const std::vector<double>& getHistBest()  const { return histBest_; }
    const std::vector<double>& getHistAvg()   const { return histAvg_; }
    const std::vector<double>& getHistWorst() const { return histWorst_; }
    const std::vector<double>& getHistory()   const { return histBest_; }
    const GP_Individual& getBestGen0() const { return bestGen0_; }
    const std::vector<GP_ParetoPoint>& getPareto() const { return pareto_; }
    bool hasBestGen0() const { return hasBestGen0_; }






private:
    Instance& inst;
    GPEA_Params   P;
    mutable std::mt19937 rng;

    gp::CPMPrecalc cpm{};
    ImopseBounds   bounds{};

    std::vector<double> histHV_;
    double rand01();
    int    randInt(int lo, int hi);

    GP_Individual evaluate(const GP_Individual& ind) const;

    void initPopulation(std::vector<GP_Individual>& pop);
    const GP_Individual& tournament(const std::vector<GP_Individual>& pop, int k);

    void crossover(GPTree& a, GPTree& b, bool isResTree);
    void mutateParam(GPTree& t, bool isResTree);
    void mutateStruct(GPTree& t, bool isResTree);
    int  pickRandomNode(const GPTree& t);
    void clampDepth(GPTree& t, int maxDepth, bool isResTree);

    std::vector<double> histBest_;
    std::vector<double> histAvg_;
    std::vector<double> histWorst_;
    std::vector<GP_ParetoPoint> pareto_;
    void updatePareto(const GP_Individual& ind);

    bool  hasSeed_ = false;
    GPTree seedTask_;
    GPTree seedRes_;
    bool hasBestGen0_ = false;
    GP_Individual bestGen0_{};

    bool dominatesMO(const GP_Individual& a, const GP_Individual& b) const;
    std::vector<std::vector<int>> nonDominatedSort(std::vector<GP_Individual>& pop) const;
    void calcCrowdingDistance(std::vector<GP_Individual>& pop, const std::vector<int>& front) const;
    const GP_Individual& tournamentMO(const std::vector<GP_Individual>& pop, int k);
    std::vector<GP_Individual> selectNextPopulationNSGA2(std::vector<GP_Individual>& combined);

};
