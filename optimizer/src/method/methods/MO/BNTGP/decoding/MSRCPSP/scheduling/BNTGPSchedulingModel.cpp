#include "BNTGPSchedulingModel.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace bntgp::decoding::msrcpsp::scheduling
{
    namespace
    {
        [[nodiscard]]
        int matchedLevel(
            const domain::Task& task,
            const domain::Resource& resource)
        {
            if (task.requiredSkills.empty())
            {
                const auto skillIt = resource.skills.find(
                    task.reqSkill
                );

                return skillIt != resource.skills.end()
                    ? skillIt->second
                    : 0;
            }

            int result = 0;

            for (const domain::SkillRequirement& requirement :
                 task.requiredSkills)
            {
                const auto skillIt = resource.skills.find(
                    requirement.skill
                );

                if (skillIt != resource.skills.end())
                {
                    result += skillIt->second;
                }
            }

            return result;
        }
    }

    BNTGPSchedulingModel::BNTGPSchedulingModel(
        const domain::Instance& instance,
        const scheduling::FeatureScaling& scaling,
        const scheduling::CPMPrecalc& cpm)
        : taskCount_(instance.tasks.size()),
          resourceCount_(instance.resources.size())
    {
        const int taskCount = static_cast<int>(taskCount_);
        const int resourceCount = static_cast<int>(resourceCount_);

        resourceIndexById_.reserve(resourceCount_ * 2U);
        skillLevelsBySkill_.reserve(16U);
        resourceIdsBySkill_.reserve(16U);

        for (int resourceIndex = 0;
             resourceIndex < resourceCount;
             ++resourceIndex)
        {
            resourceIndexById_[
                instance.resources[resourceIndex].id
            ] = resourceIndex;
        }

        for (std::size_t resourceIndex = 0U;
             resourceIndex < resourceCount_;
             ++resourceIndex)
        {
            const domain::Resource& resource =
                instance.resources[resourceIndex];

            for (const auto& [skill, level] : resource.skills)
            {
                if (level > 0)
                {
                    std::vector<int>& levels =
                        skillLevelsBySkill_[skill];

                    if (levels.empty())
                    {
                        levels.assign(resourceCount_, 0);
                    }

                    levels[resourceIndex] = level;
                }

                resourceIdsBySkill_[skill].push_back(
                    resource.id
                );
            }
        }

        baseIndegrees_.assign(taskCount_, 0);
        successors_.assign(taskCount_, {});
        feasibleResourceCountByTask_.assign(taskCount_, 0);
        taskResourceCounts_.assign(taskCount_, 0.0);
        averageResourceCosts_.assign(
            taskCount_,
            std::numeric_limits<double>::infinity()
        );
        candidateResourceIndicesByTask_.assign(taskCount_, {});
        scoringResourceIndicesByTask_.assign(taskCount_, {});
        matchedLevelByTaskResource_.assign(
            taskCount_ * resourceCount_,
            0
        );
        reservePressureWeightByTask_.assign(taskCount_, 0.0);
        criticalReserveWeightByTask_.assign(taskCount_, 0.0);
        familyIdByTask_.assign(taskCount_, -1);
        initialReservePressureByResource_.assign(resourceCount_, 0.0);
        initialCriticalReserveByResource_.assign(resourceCount_, 0.0);

        for (int taskIndex = 0;
             taskIndex < taskCount;
             ++taskIndex)
        {
            for (const int predecessorId :
                 instance.tasks[taskIndex].predecessors)
            {
                if (instance.idToIndex.count(predecessorId) > 0U)
                {
                    ++baseIndegrees_[taskIndex];
                }
            }
        }

        for (int taskIndex = 0;
             taskIndex < taskCount;
             ++taskIndex)
        {
            for (const int predecessorId :
                 instance.tasks[taskIndex].predecessors)
            {
                const auto predecessorIt =
                    instance.idToIndex.find(predecessorId);

                if (predecessorIt != instance.idToIndex.end())
                {
                    successors_[predecessorIt->second].push_back(
                        taskIndex
                    );
                }
            }
        }

        for (int taskIndex = 0;
             taskIndex < taskCount;
             ++taskIndex)
        {
            const domain::Task& task = instance.tasks[taskIndex];
            const std::size_t base =
                static_cast<std::size_t>(taskIndex) * resourceCount_;

            for (int resourceIndex = 0;
                 resourceIndex < resourceCount;
                 ++resourceIndex)
            {
                matchedLevelByTaskResource_[
                    base + static_cast<std::size_t>(resourceIndex)
                ] = matchedLevel(
                    task,
                    instance.resources[resourceIndex]
                );
            }
        }

        for (int taskIndex = 0;
             taskIndex < taskCount;
             ++taskIndex)
        {
            const domain::Task& task = instance.tasks[taskIndex];
            std::vector<int>& scoringResources =
                scoringResourceIndicesByTask_[taskIndex];

            if (!task.capableResourceIndices.empty())
            {
                scoringResources.reserve(
                    task.capableResourceIndices.size()
                );

                for (const int resourceIndex :
                     task.capableResourceIndices)
                {
                    if (resourceIndex >= 0 &&
                        resourceIndex < resourceCount)
                    {
                        scoringResources.push_back(resourceIndex);
                    }
                }
            }
            else
            {
                scoringResources.reserve(resourceCount_);

                for (int resourceIndex = 0;
                     resourceIndex < resourceCount;
                     ++resourceIndex)
                {
                    if (task.reqLevel > 0)
                    {
                        const domain::Resource& resource =
                            instance.resources[resourceIndex];
                        const auto skillIt = resource.skills.find(
                            task.reqSkill
                        );
                        const int level = skillIt != resource.skills.end()
                            ? skillIt->second
                            : 0;

                        if (level < task.reqLevel)
                        {
                            continue;
                        }
                    }

                    scoringResources.push_back(resourceIndex);
                }
            }
        }

        for (int taskIndex = 0;
             taskIndex < taskCount;
             ++taskIndex)
        {
            const domain::Task& task = instance.tasks[taskIndex];
            const int requiredLevel = std::max(
                0,
                task.totalRequiredLevel()
            );

            if (requiredLevel <= 0)
            {
                feasibleResourceCountByTask_[taskIndex] =
                    resourceCount;
                taskResourceCounts_[taskIndex] =
                    static_cast<double>(resourceCount);
                averageResourceCosts_[taskIndex] =
                    std::numeric_limits<double>::infinity();
                continue;
            }

            if (!task.capableResourceIndices.empty())
            {
                feasibleResourceCountByTask_[taskIndex] =
                    static_cast<int>(
                        task.capableResourceIndices.size()
                    );
                taskResourceCounts_[taskIndex] =
                    static_cast<double>(
                        task.capableResourceIndices.size()
                    );

                double salarySum = 0.0;
                int validResourceCount = 0;

                for (const int resourceIndex :
                     task.capableResourceIndices)
                {
                    if (resourceIndex < 0 ||
                        resourceIndex >= resourceCount)
                    {
                        continue;
                    }

                    salarySum +=
                        instance.resources[resourceIndex].salary;
                    ++validResourceCount;
                }

                averageResourceCosts_[taskIndex] =
                    validResourceCount > 0
                        ? salarySum /
                          static_cast<double>(validResourceCount)
                        : std::numeric_limits<double>::infinity();
                continue;
            }

            int feasibleCount = 0;
            double salarySum = 0.0;
            int validResourceCount = 0;

            for (const domain::Resource& resource :
                 instance.resources)
            {
                if (!task.canBeDoneBy(resource))
                {
                    continue;
                }

                ++feasibleCount;
                salarySum += resource.salary;
                ++validResourceCount;
            }

            feasibleResourceCountByTask_[taskIndex] =
                feasibleCount;
            taskResourceCounts_[taskIndex] =
                static_cast<double>(feasibleCount);
            averageResourceCosts_[taskIndex] =
                validResourceCount > 0
                    ? salarySum /
                      static_cast<double>(validResourceCount)
                    : std::numeric_limits<double>::infinity();
        }

        std::unordered_map<std::string, int> familyIndex;
        familyIndex.reserve(taskCount_ * 2U);

        for (int taskIndex = 0;
             taskIndex < taskCount;
             ++taskIndex)
        {
            const std::string key =
                instance.tasks[taskIndex].requirementKey();

            const auto familyIt = familyIndex.find(key);

            if (familyIt == familyIndex.end())
            {
                const int familyId =
                    static_cast<int>(familyIndex.size());

                familyIndex.emplace(key, familyId);
                familyIdByTask_[taskIndex] = familyId;
            }
            else
            {
                familyIdByTask_[taskIndex] = familyIt->second;
            }
        }

        familyCount_ = std::max(
            1,
            static_cast<int>(familyIndex.size())
        );

        initialFamilyPressureByResourceFamily_.assign(
            resourceCount_ * static_cast<std::size_t>(familyCount_),
            0.0
        );

        for (int taskIndex = 0;
             taskIndex < taskCount;
             ++taskIndex)
        {
            const domain::Task& task = instance.tasks[taskIndex];
            const int requiredLevel = std::max(
                0,
                task.totalRequiredLevel()
            );
            const int feasibleCount = std::max(
                1,
                feasibleResourceCountByTask_[taskIndex]
            );

            double cheapest =
                std::numeric_limits<double>::infinity();
            double secondCheapest =
                std::numeric_limits<double>::infinity();

            if (!task.capableResourceIndices.empty())
            {
                for (const int resourceIndex :
                     task.capableResourceIndices)
                {
                    if (resourceIndex < 0 ||
                        resourceIndex >= resourceCount)
                    {
                        continue;
                    }

                    const double salary =
                        instance.resources[resourceIndex].salary;

                    if (salary < cheapest)
                    {
                        secondCheapest = cheapest;
                        cheapest = salary;
                    }
                    else if (salary < secondCheapest)
                    {
                        secondCheapest = salary;
                    }
                }
            }
            else
            {
                for (int resourceIndex = 0;
                     resourceIndex < resourceCount;
                     ++resourceIndex)
                {
                    const domain::Resource& resource =
                        instance.resources[resourceIndex];

                    if (requiredLevel > 0 &&
                        !task.canBeDoneBy(resource))
                    {
                        continue;
                    }

                    const double salary = resource.salary;

                    if (salary < cheapest)
                    {
                        secondCheapest = cheapest;
                        cheapest = salary;
                    }
                    else if (salary < secondCheapest)
                    {
                        secondCheapest = salary;
                    }
                }
            }

            if (!std::isfinite(secondCheapest))
            {
                secondCheapest = cheapest;
            }

            const double priceGap = std::max(
                0.0,
                secondCheapest - cheapest
            );
            const double reserveWeight =
                (static_cast<double>(task.duration) * priceGap) /
                static_cast<double>(feasibleCount);

            double criticalLength = 0.0;
            double slack = 0.0;
            double descendantCount = 0.0;

            if (taskIndex >= 0 &&
                taskIndex < static_cast<int>(cpm.critLen.size()))
            {
                criticalLength = cpm.critLen[taskIndex];
            }

            if (taskIndex >= 0 &&
                taskIndex < static_cast<int>(cpm.slack.size()))
            {
                slack = cpm.slack[taskIndex];
            }

            if (taskIndex >= 0 &&
                taskIndex < static_cast<int>(cpm.descCount.size()))
            {
                descendantCount = cpm.descCount[taskIndex];
            }

            const double normalizedCriticalLength =
                scaling.maxCritLen > 0.0
                    ? criticalLength / scaling.maxCritLen
                    : 0.0;
            const double normalizedSlack =
                scaling.maxSlackPos > 0.0
                    ? std::max(0.0, slack) /
                      scaling.maxSlackPos
                    : 0.0;
            const double normalizedDescendantCount =
                scaling.maxNumTasks > 0.0
                    ? descendantCount / scaling.maxNumTasks
                    : 0.0;

            const double structuralPressure =
                (1.0 + normalizedCriticalLength +
                 normalizedDescendantCount) /
                (1.0 + normalizedSlack);
            const double criticalReserveWeight =
                reserveWeight * structuralPressure;
            const int familyId = familyIdByTask_[taskIndex];

            reservePressureWeightByTask_[taskIndex] =
                reserveWeight;
            criticalReserveWeightByTask_[taskIndex] =
                criticalReserveWeight;

            std::vector<int>& candidateResources =
                candidateResourceIndicesByTask_[taskIndex];

            if (requiredLevel <= 0)
            {
                candidateResources.reserve(resourceCount_);

                for (int resourceIndex = 0;
                     resourceIndex < resourceCount;
                     ++resourceIndex)
                {
                    candidateResources.push_back(resourceIndex);
                    initialReservePressureByResource_[resourceIndex] +=
                        reserveWeight;
                    initialCriticalReserveByResource_[resourceIndex] +=
                        criticalReserveWeight;
                    initialFamilyPressureByResourceFamily_[
                        static_cast<std::size_t>(resourceIndex) *
                        static_cast<std::size_t>(familyCount_) +
                        static_cast<std::size_t>(familyId)
                    ] += reserveWeight;
                }

                continue;
            }

            candidateResources.reserve(
                static_cast<std::size_t>(std::max(1, feasibleCount))
            );

            if (!task.capableResourceIndices.empty())
            {
                for (const int resourceIndex :
                     task.capableResourceIndices)
                {
                    if (resourceIndex < 0 ||
                        resourceIndex >= resourceCount)
                    {
                        continue;
                    }

                    candidateResources.push_back(resourceIndex);
                    initialReservePressureByResource_[resourceIndex] +=
                        reserveWeight;
                    initialCriticalReserveByResource_[resourceIndex] +=
                        criticalReserveWeight;
                    initialFamilyPressureByResourceFamily_[
                        static_cast<std::size_t>(resourceIndex) *
                        static_cast<std::size_t>(familyCount_) +
                        static_cast<std::size_t>(familyId)
                    ] += reserveWeight;
                }
            }
            else
            {
                for (int resourceIndex = 0;
                     resourceIndex < resourceCount;
                     ++resourceIndex)
                {
                    const domain::Resource& resource =
                        instance.resources[resourceIndex];

                    if (requiredLevel > 0 &&
                        !task.canBeDoneBy(resource))
                    {
                        continue;
                    }

                    candidateResources.push_back(resourceIndex);
                    initialReservePressureByResource_[resourceIndex] +=
                        reserveWeight;
                    initialCriticalReserveByResource_[resourceIndex] +=
                        criticalReserveWeight;
                    initialFamilyPressureByResourceFamily_[
                        static_cast<std::size_t>(resourceIndex) *
                        static_cast<std::size_t>(familyCount_) +
                        static_cast<std::size_t>(familyId)
                    ] += reserveWeight;
                }
            }
        }
    }

    std::size_t BNTGPSchedulingModel::taskCount() const noexcept
    {
        return taskCount_;
    }

    std::size_t BNTGPSchedulingModel::resourceCount() const noexcept
    {
        return resourceCount_;
    }

    int BNTGPSchedulingModel::familyCount() const noexcept
    {
        return familyCount_;
    }

    int BNTGPSchedulingModel::resourceIndex(
        const int resourceId) const noexcept
    {
        const auto resourceIt =
            resourceIndexById_.find(resourceId);

        return resourceIt != resourceIndexById_.end()
            ? resourceIt->second
            : -1;
    }

    void BNTGPSchedulingModel::buildSkillStepCache(
        const domain::Instance& instance,
        const int now,
        std::unordered_map<
            std::string,
            scheduling::SkillStepInfo
        >& output) const
    {
        output.clear();
        output.reserve(skillLevelsBySkill_.size());

        for (const auto& [skill, levels] :
             skillLevelsBySkill_)
        {
            int maximumLevel = 0;

            for (const int level : levels)
            {
                if (level > maximumLevel)
                {
                    maximumLevel = level;
                }
            }

            scheduling::SkillStepInfo info;
            info.maxFreeLevel = 0;

            if (maximumLevel <= 0)
            {
                output.emplace(skill, std::move(info));
                continue;
            }

            const int infiniteWait =
                std::numeric_limits<int>::max();
            const double infiniteCost =
                std::numeric_limits<double>::infinity();

            std::vector<int> exactMinimumWait(
                static_cast<std::size_t>(maximumLevel + 1),
                infiniteWait
            );
            std::vector<double> exactCheapest(
                static_cast<std::size_t>(maximumLevel + 1),
                infiniteCost
            );
            std::vector<double> exactSecondCheapest(
                static_cast<std::size_t>(maximumLevel + 1),
                infiniteCost
            );

            for (int resourceIndex = 0;
                 resourceIndex < static_cast<int>(resourceCount_);
                 ++resourceIndex)
            {
                const int level = levels[
                    static_cast<std::size_t>(resourceIndex)
                ];

                if (level <= 0)
                {
                    continue;
                }

                const domain::Resource& resource =
                    instance.resources[resourceIndex];
                const int wait = resource.busyUntil <= now
                    ? 0
                    : resource.busyUntil - now;

                if (wait < exactMinimumWait[level])
                {
                    exactMinimumWait[level] = wait;
                }

                if (wait == 0)
                {
                    if (level > info.maxFreeLevel)
                    {
                        info.maxFreeLevel = level;
                    }

                    const double salary = resource.salary;

                    if (salary < exactCheapest[level])
                    {
                        exactSecondCheapest[level] =
                            exactCheapest[level];
                        exactCheapest[level] = salary;
                    }
                    else if (salary < exactSecondCheapest[level])
                    {
                        exactSecondCheapest[level] = salary;
                    }
                }
            }

            info.minWaitAtLeast.assign(
                static_cast<std::size_t>(maximumLevel + 1),
                infiniteWait
            );
            info.cheapestAtLeast.assign(
                static_cast<std::size_t>(maximumLevel + 1),
                infiniteCost
            );
            info.secondCheapestAtLeast.assign(
                static_cast<std::size_t>(maximumLevel + 1),
                infiniteCost
            );

            int carriedWait = infiniteWait;
            double carriedCheapest = infiniteCost;
            double carriedSecondCheapest = infiniteCost;

            const auto feedCost = [
                &carriedCheapest,
                &carriedSecondCheapest
            ](const double value)
            {
                if (!std::isfinite(value))
                {
                    return;
                }

                if (value < carriedCheapest)
                {
                    carriedSecondCheapest = carriedCheapest;
                    carriedCheapest = value;
                }
                else if (value < carriedSecondCheapest)
                {
                    carriedSecondCheapest = value;
                }
            };

            for (int level = maximumLevel;
                 level >= 1;
                 --level)
            {
                if (exactMinimumWait[level] < carriedWait)
                {
                    carriedWait = exactMinimumWait[level];
                }

                feedCost(exactCheapest[level]);
                feedCost(exactSecondCheapest[level]);

                info.minWaitAtLeast[level] = carriedWait;
                info.cheapestAtLeast[level] = carriedCheapest;
                info.secondCheapestAtLeast[level] =
                    carriedSecondCheapest;
            }

            output.emplace(skill, std::move(info));
        }
    }

    const std::unordered_map<int, int>&
    BNTGPSchedulingModel::resourceIndexById() const noexcept
    {
        return resourceIndexById_;
    }

    const std::unordered_map<std::string, std::vector<int>>&
    BNTGPSchedulingModel::resourceIdsBySkill() const noexcept
    {
        return resourceIdsBySkill_;
    }

    const std::unordered_map<std::string, std::vector<int>>&
    BNTGPSchedulingModel::skillLevelsBySkill() const noexcept
    {
        return skillLevelsBySkill_;
    }

    const std::vector<int>&
    BNTGPSchedulingModel::baseIndegrees() const noexcept
    {
        return baseIndegrees_;
    }

    const std::vector<std::vector<int>>&
    BNTGPSchedulingModel::successors() const noexcept
    {
        return successors_;
    }

    const std::vector<double>&
    BNTGPSchedulingModel::taskResourceCounts() const noexcept
    {
        return taskResourceCounts_;
    }

    const std::vector<double>&
    BNTGPSchedulingModel::averageResourceCosts() const noexcept
    {
        return averageResourceCosts_;
    }

    const std::vector<int>&
    BNTGPSchedulingModel::candidateResourceIndices(
        const int taskIndex) const noexcept
    {
        return candidateResourceIndicesByTask_[
            static_cast<std::size_t>(taskIndex)
        ];
    }

    const std::vector<int>&
    BNTGPSchedulingModel::scoringResourceIndices(
        const int taskIndex) const noexcept
    {
        return scoringResourceIndicesByTask_[
            static_cast<std::size_t>(taskIndex)
        ];
    }

    const std::vector<int>&
    BNTGPSchedulingModel::matchedLevelByTaskResource() const noexcept
    {
        return matchedLevelByTaskResource_;
    }

    const std::vector<double>&
    BNTGPSchedulingModel::reservePressureWeightByTask() const noexcept
    {
        return reservePressureWeightByTask_;
    }

    const std::vector<double>&
    BNTGPSchedulingModel::criticalReserveWeightByTask() const noexcept
    {
        return criticalReserveWeightByTask_;
    }

    const std::vector<int>&
    BNTGPSchedulingModel::familyIdByTask() const noexcept
    {
        return familyIdByTask_;
    }

    const std::vector<double>&
    BNTGPSchedulingModel::initialReservePressureByResource() const noexcept
    {
        return initialReservePressureByResource_;
    }

    const std::vector<double>&
    BNTGPSchedulingModel::initialCriticalReserveByResource() const noexcept
    {
        return initialCriticalReserveByResource_;
    }

    const std::vector<double>&
    BNTGPSchedulingModel::initialFamilyPressureByResourceFamily() const noexcept
    {
        return initialFamilyPressureByResourceFamily_;
    }
}
