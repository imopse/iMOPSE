#include "BNTGPPairTreeScorer.hpp"

#include "../../../gp/GPTreeEvaluator.hpp"


namespace bntgp::decoding::msrcpsp::scheduling
{
    BNTGPPairTreeScorer::
    BNTGPPairTreeScorer(
        const gp::GPTree& tree) noexcept
        : tree_(tree)
    {
        usedFeatures_.fill(false);

        for (const gp::GPNode& node : tree_.nodes())
        {
            if (node.kind != gp::NodeKind::FEATURE)
            {
                continue;
            }

            if (!gp::isValidFeature(node.feature))
            {
                continue;
            }

            usedFeatures_[
                gp::toFeatureIndex(node.feature)
            ] = true;
        }
    }

    bool BNTGPPairTreeScorer::usesFeature(
        const gp::FeatureId feature) const noexcept
    {
        if (!gp::isValidFeature(feature))
        {
            return false;
        }

        return usedFeatures_[
            gp::toFeatureIndex(feature)
        ];
    }

    double BNTGPPairTreeScorer::score(
        const scheduling::Features& features) noexcept
    {
        taskValues_[gp::FeatureId::DURATION] =
            features.duration;

        taskValues_[gp::FeatureId::REQ_LEVEL] =
            features.reqLevel;

        taskValues_[gp::FeatureId::AVAIL_SKILL] =
            features.availSkill;

        taskValues_[gp::FeatureId::CRITLEN] =
            features.critLen;

        taskValues_[gp::FeatureId::SLACK] =
            features.slack;

        taskValues_[gp::FeatureId::DESC_COUNT] =
            features.descCount;

        taskValues_[
            gp::FeatureId::TASK_RELEASE_PRESSURE
        ] = features.taskReleasePressure;

        taskValues_[
            gp::FeatureId::TASK_CRITICAL_PRESSURE
        ] = features.taskCriticalPressure;

        taskValues_[gp::FeatureId::AVAIL_GAP] =
            features.availGap;

        taskValues_[
            gp::FeatureId::CHEAPEST_COST_NOW
        ] = features.cheapestCostNow;

        taskValues_[
            gp::FeatureId::COST_PER_SKILL_NOW
        ] = features.costPerSkillNow;

        taskValues_[gp::FeatureId::TASK_RES_COUNT] =
            features.taskResCount;

        taskValues_[gp::FeatureId::AVG_RES_COST] =
            features.avgResCostForSkill;

        taskValues_[gp::FeatureId::UNSCHED_TASKS] =
            features.unschedTasks;

        taskValues_[
            gp::FeatureId::MIN_FEASIBLE_COST_NOW
        ] = features.minFeasibleCostNow;

        taskValues_[gp::FeatureId::COST_REGRET_NOW] =
            features.costRegretNow;

        resourceValues_[gp::FeatureId::RES_WAGE] =
            features.resWage;

        resourceValues_[gp::FeatureId::RES_SKILL_LEVEL] =
            features.resSkillLevel;

        resourceValues_[gp::FeatureId::RES_IDLE_TIME] =
            features.resIdleTime;

        resourceValues_[gp::FeatureId::RES_CAN_START_NOW] =
            features.resCanStartNow;

        resourceValues_[gp::FeatureId::RES_UTILIZATION] =
            features.resUtilization;

        resourceValues_[gp::FeatureId::RES_WAGE_PER_LEVEL] =
            features.resWagePerLevel;

        resourceValues_[gp::FeatureId::RES_ASSIGN_COST] =
            features.resAssignCost;

        resourceValues_[
            gp::FeatureId::RES_ASSIGN_PREMIUM_ALL
        ] = features.resAssignPremiumAll;

        resourceValues_[
            gp::FeatureId::RES_RESERVE_PRESSURE
        ] = features.resReservePressure;

        resourceValues_[
            gp::FeatureId::RES_FAMILY_MISMATCH
        ] = features.resFamilyMismatch;

        resourceValues_[
            gp::FeatureId::RES_FUTURE_BRANCH_FIT
        ] = features.resFutureBranchFit;

        resourceValues_[
            gp::FeatureId::RES_BOTTLENECK_PRESERVATION
        ] = features.resBottleneckPreservation;

        resourceValues_[
            gp::FeatureId::RES_SPECIALIST_MISUSE
        ] = features.resSpecialistMisuse;

        resourceValues_[
            gp::FeatureId::RES_RELATIVE_WAGE
        ] = features.resRelativeWage;

        const gp::FeatureValuesView featureValues{
            taskValues_,
            resourceValues_
        };

        return gp::evaluateTree(
            tree_,
            featureValues
        );
    }
}
