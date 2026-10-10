#pragma once

#include "BNTGPPairTreeScorer.hpp"

namespace bntgp::decoding::msrcpsp::scheduling
{
    struct BNTGPFeatureRequirements final
    {
        explicit BNTGPFeatureRequirements(
            const BNTGPPairTreeScorer& scorer) noexcept
            : taskCriticalLength(
                  scorer.usesFeature(gp::FeatureId::CRITLEN)
              ),
              taskSlack(
                  scorer.usesFeature(gp::FeatureId::SLACK)
              ),
              taskDescendantCount(
                  scorer.usesFeature(gp::FeatureId::DESC_COUNT)
              ),
              taskReleasePressure(
                  scorer.usesFeature(
                      gp::FeatureId::TASK_RELEASE_PRESSURE
                  )
              ),
              taskCriticalPressure(
                  scorer.usesFeature(
                      gp::FeatureId::TASK_CRITICAL_PRESSURE
                  )
              ),
              taskAvailableSkill(
                  scorer.usesFeature(gp::FeatureId::AVAIL_SKILL)
              ),
              taskAvailabilityGap(
                  scorer.usesFeature(gp::FeatureId::AVAIL_GAP)
              ),
              taskCheapestCostNow(
                  scorer.usesFeature(
                      gp::FeatureId::CHEAPEST_COST_NOW
                  )
              ),
              taskCostPerSkillNow(
                  scorer.usesFeature(
                      gp::FeatureId::COST_PER_SKILL_NOW
                  )
              ),
              taskMinimumFeasibleCostNow(
                  scorer.usesFeature(
                      gp::FeatureId::MIN_FEASIBLE_COST_NOW
                  )
              ),
              taskCostRegretNow(
                  scorer.usesFeature(
                      gp::FeatureId::COST_REGRET_NOW
                  )
              ),
              taskResourceCount(
                  scorer.usesFeature(gp::FeatureId::TASK_RES_COUNT)
              ),
              taskAverageResourceCost(
                  scorer.usesFeature(gp::FeatureId::AVG_RES_COST)
              ),
              unscheduledTaskCount(
                  scorer.usesFeature(gp::FeatureId::UNSCHED_TASKS)
              ),
              resourceReservePressure(
                  scorer.usesFeature(
                      gp::FeatureId::RES_RESERVE_PRESSURE
                  )
              ),
              resourceFamilyMismatch(
                  scorer.usesFeature(
                      gp::FeatureId::RES_FAMILY_MISMATCH
                  )
              ),
              resourceFutureBranchFit(
                  scorer.usesFeature(
                      gp::FeatureId::RES_FUTURE_BRANCH_FIT
                  )
              ),
              resourceBottleneckPreservation(
                  scorer.usesFeature(
                      gp::FeatureId::RES_BOTTLENECK_PRESERVATION
                  )
              ),
              resourceSpecialistMisuse(
                  scorer.usesFeature(
                      gp::FeatureId::RES_SPECIALIST_MISUSE
                  )
              )
        {
        }

        [[nodiscard]]
        bool needsTaskStructureFeatures() const noexcept
        {
            return taskCriticalLength ||
                taskSlack ||
                taskDescendantCount ||
                taskReleasePressure ||
                taskCriticalPressure;
        }

        [[nodiscard]]
        bool needsTaskAvailabilityFeatures() const noexcept
        {
            return taskAvailableSkill ||
                taskAvailabilityGap ||
                taskCheapestCostNow ||
                taskCostPerSkillNow ||
                taskMinimumFeasibleCostNow ||
                taskCostRegretNow;
        }

        [[nodiscard]]
        bool needsTaskCostNowFeatures() const noexcept
        {
            return taskCheapestCostNow ||
                taskCostPerSkillNow ||
                taskMinimumFeasibleCostNow ||
                taskCostRegretNow;
        }

        bool taskCriticalLength{false};
        bool taskSlack{false};
        bool taskDescendantCount{false};
        bool taskReleasePressure{false};
        bool taskCriticalPressure{false};
        bool taskAvailableSkill{false};
        bool taskAvailabilityGap{false};
        bool taskCheapestCostNow{false};
        bool taskCostPerSkillNow{false};
        bool taskMinimumFeasibleCostNow{false};
        bool taskCostRegretNow{false};
        bool taskResourceCount{false};
        bool taskAverageResourceCost{false};
        bool unscheduledTaskCount{false};
        bool resourceReservePressure{false};
        bool resourceFamilyMismatch{false};
        bool resourceFutureBranchFit{false};
        bool resourceBottleneckPreservation{false};
        bool resourceSpecialistMisuse{false};
    };
}
