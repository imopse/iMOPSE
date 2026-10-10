#pragma once
#include <vector>
#include <random>
#include <limits>
#include <memory>
#include <utility>
#include <algorithm>
#include <string>
#include <cstdint>
#include <unordered_set>

#include "../domain/Instance.hpp"
#include "../scheduler/Scheduler.hpp"
#include "../rules/GPTreeRule.hpp"
#include "GPTree.hpp"
#include "Precompute.hpp"
#include "Normalization.hpp"

class CScheduler;

namespace gphh_so {

    struct GPEA_Params {
        size_t   popSize = 40;
        size_t   generations = 100;
        double   pCrossover = 0.9;
        double   pMutation = 0.2;

        double   pSubtreeMutation = 0.34;
        double   pPointMutation = 0.33;
        double   pHoistMutation = 0.33;
        double   pFeatureGuidedPointMutation = 0.0;
        double   pMacroSubtreeMutation = 0.0;

        double   pSubtreeCrossover = 1.0;
        double   pFeatureAwareCrossover = 0.0;

        int      maxDepth = 4;
        uint64_t seed = 1234567;
        double   weight = 0.5;
        bool     useNormalization = true;
        int      tournamentK = 3;
        size_t   eliteCount = 1;
        bool     useImopseEvaluate = false;
        bool     useSinglePairTree = false;
        bool     useGeneLevelMutation = false;
        bool     logPopulationDiversity = false;
        bool     logMutationDebug = false;
        bool     avoidClonesInPopulation = false;
        int      cloneMutationRetries = 3;
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

        std::vector<FeatureId> taskFeatureUniverse_;
        std::vector<FeatureId> resFeatureUniverse_;
        std::vector<FeatureId> pairFeatureUniverse_;
        std::vector<double> taskFeatureScores_;
        std::vector<double> resFeatureScores_;
        std::vector<double> pairFeatureScores_;
        bool adaptiveStatsReady_ = false;

        Instance& resetWorkingInstance(bool clearAssignedResources) const;
        double rand01();
        int    randInt(int lo, int hi);
        GP_Individual evaluate(const GP_Individual& ind) const;
        void initPopulation(std::vector<GP_Individual>& pop);
        GP_Individual createRandomIndividual();
        void mutateCloneRecovery(GP_Individual& ind);
        bool insertWithCloneAvoidance(
            std::vector<GP_Individual>& target,
            GP_Individual candidate,
            std::unordered_set<std::uint64_t>& seenHashes);
        const GP_Individual& tournament(const std::vector<GP_Individual>& pop, int k);

        void applyCrossover(GPTree& a, GPTree& b, bool isResTree);
        void subtreeCrossover(GPTree& a, GPTree& b, bool isResTree);
        void featureAwareCrossover(GPTree& a, GPTree& b, bool isResTree);

        double subtreeFeatureScore(const GPTree& t, int rootIndex, bool isResTree) const;
        FeatureId dominantSubtreeFeature(const GPTree& t, int rootIndex, bool isResTree) const;
        bool subtreeContainsFeature(const GPTree& t, int rootIndex, FeatureId feat) const;

        struct MutationDebugStats {
            uint64_t pointCalls = 0;
            uint64_t pointChangedTrees = 0;

            uint64_t guidedCalls = 0;
            uint64_t guidedChangedTrees = 0;
            uint64_t guidedFallbackToPoint = 0;

            uint64_t geneLevelCalls = 0;
            uint64_t geneLevelChangedTrees = 0;

            uint64_t featureNodesVisited = 0;
            uint64_t featureNodesSelected = 0;
            uint64_t featureNodesChanged = 0;
            uint64_t pointGeneCalls = 0;
            uint64_t pointGeneChangedTrees = 0;

            uint64_t pointGeneVisitedNodes = 0;
            uint64_t pointGeneSelectedNodes = 0;
            uint64_t pointGeneChangedNodes = 0;

            uint64_t pointGeneChangedConst = 0;
            uint64_t pointGeneChangedFeature = 0;
            uint64_t pointGeneChangedUnary = 0;
            uint64_t pointGeneChangedBinary = 0;
            uint64_t rouletteGeneCalls = 0;
            uint64_t rouletteGeneChangedTrees = 0;

            uint64_t rouletteVisitedNodes = 0;
            uint64_t rouletteSelectedNodes = 0;
            uint64_t rouletteChangedNodes = 0;

            uint64_t roulettePointAttempts = 0;
            uint64_t roulettePointChanged = 0;

            uint64_t macroSubtreeCalls = 0;
            uint64_t macroSubtreeChangedTrees = 0;

            uint64_t cloneHits = 0;
            uint64_t cloneMutationAttempts = 0;
            uint64_t cloneRandomReplacements = 0;

            uint64_t rouletteGuidedAttempts = 0;
            uint64_t rouletteGuidedChanged = 0;

            void reset() {
                pointCalls = 0;
                pointChangedTrees = 0;

                guidedCalls = 0;
                guidedChangedTrees = 0;
                guidedFallbackToPoint = 0;

                geneLevelCalls = 0;
                geneLevelChangedTrees = 0;

                featureNodesVisited = 0;
                featureNodesSelected = 0;
                featureNodesChanged = 0;
                pointGeneCalls = 0;
                pointGeneChangedTrees = 0;

                pointGeneVisitedNodes = 0;
                pointGeneSelectedNodes = 0;
                pointGeneChangedNodes = 0;

                pointGeneChangedConst = 0;
                pointGeneChangedFeature = 0;
                pointGeneChangedUnary = 0;
                pointGeneChangedBinary = 0;
                rouletteGeneCalls = 0;
                rouletteGeneChangedTrees = 0;

                rouletteVisitedNodes = 0;
                rouletteSelectedNodes = 0;
                rouletteChangedNodes = 0;

                roulettePointAttempts = 0;
                roulettePointChanged = 0;

                macroSubtreeCalls = 0;
                macroSubtreeChangedTrees = 0;

                cloneHits = 0;
                cloneMutationAttempts = 0;
                cloneRandomReplacements = 0;

                rouletteGuidedAttempts = 0;
                rouletteGuidedChanged = 0;
            }
        };

        MutationDebugStats mutDbg_;

        void applyMutation(GPTree& t, bool isResTree);
        void subtreeMutation(GPTree& t, bool isResTree);
        void pointMutation(GPTree& t, bool isResTree);
        void pointMutationPerGene(GPTree& t, bool isResTree);
        bool pointMutateNodeAt(GPTree& t, int idx, bool isResTree);
        void hoistMutation(GPTree& t, bool isResTree);
        void featureGuidedPointMutation(GPTree& t, bool isResTree);
        void featureGuidedPointMutationPerGene(GPTree& t, bool isResTree);
        bool featureGuidedMutateNodeAt(GPTree& t, int idx, bool isResTree);
        void geneLevelMutationRoulette(GPTree& t, bool isResTree);

        void refreshAdaptiveFeatureStats(const std::vector<GP_Individual>& pop);
        void accumulateFeatureCounts(const GPTree& t,
            const std::vector<FeatureId>& universe,
            std::vector<double>& counts) const;
        const std::vector<FeatureId>& mutationFeaturePool(bool isResTree) const;
        const std::vector<double>& mutationFeatureScores(bool isResTree) const;
        int pickGuidedFeatureNode(const GPTree& t, bool isResTree);
        FeatureId pickReplacementFeature(FeatureId current, bool isResTree);

        int  pickRandomNode(const GPTree& t);
        int  nodeArity(const GPNode& n) const;
        void clampDepth(GPTree& t, int maxDepth, bool isResTree);
    };

}