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

class CScheduler;

struct GPEA_Params {
    size_t   popSize = 40;
    size_t   generations = 100;
    double   pCrossover = 0.9;
    double   pMutParam = 0.1;
    double   pMutStruct = 0.05;
    double   pMutMacroSubtree = 0.01;
    int      maxDepth = 4;
    uint64_t seed = 1234567;
    int      tournamentK = 3;
    bool     useSinglePairTree = false;

    bool     logNodeDistribution = false;
    size_t   nodeStatsEvery = 25;
    bool     nodeStatsUseArchive = false;
};

struct GP_Individual {
    GPTree taskTree;
    GPTree resTree;
    double fitness = std::numeric_limits<double>::infinity();
    int    makespan = 0;
    double cost = 0.0;
    double msNorm = 0.0;
    double costNorm = 0.0;
    size_t selectedCount = 0;
};

class TreeEA {
public:
    TreeEA(Instance& I, const GPEA_Params& P, CScheduler* imopseScheduler, bool isTAProblem);
    ~TreeEA();

    void run();

    const std::vector<GP_Individual>& getArchive() const { return archive_; }

private:
    Instance& inst;
    mutable Instance workInst_;
    GPEA_Params P;
    mutable std::mt19937 rng;
    gp::CPMPrecalc cpm{};
    ImopseBounds bounds{};
    CScheduler* imopseSch_ = nullptr;
    bool imopseIsTA_ = false;

    Instance& resetWorkingInstance(bool clearAssignedResources) const;
    double rand01();
    int    randInt(int lo, int hi);
    GP_Individual evaluate(const GP_Individual& ind) const;
    void initPopulation(std::vector<GP_Individual>& pop);
    void crossover(GPTree& a, GPTree& b, bool isResTree);
    void mutateParam(GPTree& t, bool isResTree);
    void mutateStruct(GPTree& t, bool isResTree);
    void mutateMacroSubtreeReplace(GPTree& t, bool isResTree);
    int  pickRandomNode(const GPTree& t);
    void clampDepth(GPTree& t, int maxDepth, bool isResTree);

    std::vector<GP_Individual> archive_;

    void copyToArchiveWithFiltering(const std::vector<GP_Individual>& individuals);
    double objNorm(const GP_Individual& x, int objId) const;
    std::vector<std::pair<int, int>> selectParentsBNTGA(int populationSize);
};