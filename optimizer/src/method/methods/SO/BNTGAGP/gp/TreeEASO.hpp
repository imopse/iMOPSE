#pragma once
#include <vector>
#include <random>
#include <limits>
#include <memory>
#include <utility>
#include <algorithm>
#include <string>

#include "../domain/Instance.hpp"
#include "../scheduler/Scheduler.hpp"
#include "../rules/GPTreeRule.hpp"
#include "GPTree.hpp"
#include "Precompute.hpp"
#include "Normalization.hpp"

class CScheduler;

namespace gpbntga_so {

    struct GPEA_Params {
        size_t   popSize = 40;
        size_t   generations = 100;
        double   pCrossover = 0.9;
        double   pMutParam = 0.1;
        double   pMutStruct = 0.05;
        double   pMutMacroSubtree = 0.01;
        int      maxDepth = 4;
        uint64_t seed = 1234567;
        double   weight = 0.5;
        bool     useNormalization = true;
        int      tournamentK = 3;
        size_t   eliteCount = 1;
        bool     useImopseEvaluate = false;
        bool     useSinglePairTree = false;
    };

    struct GP_Individual {
        GPTree taskTree;
        GPTree resTree;
        double fitness = std::numeric_limits<double>::infinity();
        int    makespan = 0;
        double cost = 0.0;
        double msNorm = 0.0;
        double costNorm = 0.0;
    };

    class TreeEASO {
    public:
        TreeEASO(Instance& I, const GPEA_Params& P, ::CScheduler* imopseScheduler, bool isTAProblem);
        ~TreeEASO();

        GP_Individual run();

    private:
        Instance& inst;
        mutable Instance workInst_;
        GPEA_Params P;
        mutable std::mt19937 rng;
        gp::CPMPrecalc cpm{};
        ImopseBounds bounds{};
        ::CScheduler* imopseSch_ = nullptr;
        bool imopseIsTA_ = false;

        Instance& resetWorkingInstance(bool clearAssignedResources) const;
        double rand01();
        int    randInt(int lo, int hi);
        GP_Individual evaluate(const GP_Individual& ind) const;
        void initPopulation(std::vector<GP_Individual>& pop);
        const GP_Individual& tournament(const std::vector<GP_Individual>& pop, int k);
        void crossover(GPTree& a, GPTree& b, bool isResTree);
        void mutateParam(GPTree& t, bool isResTree);
        void mutateStruct(GPTree& t, bool isResTree);
        void mutateMacroSubtreeReplace(GPTree& t, bool isResTree);
        int  pickRandomNode(const GPTree& t);
        void clampDepth(GPTree& t, int maxDepth, bool isResTree);
    };

} // namespace gpbntga_so