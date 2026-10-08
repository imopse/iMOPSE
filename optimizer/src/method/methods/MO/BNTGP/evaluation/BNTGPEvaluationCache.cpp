#include "BNTGPEvaluationCache.hpp"

#include <utility>

namespace bntgp
{
    namespace
    {
        constexpr std::size_t CacheCapacityMultiplier = 8U;
    }

    void BNTGPEvaluationCache::resetForRun(
        const std::size_t populationSize)
    {
        clear();

        entries_.reserve(
            populationSize * CacheCapacityMultiplier
        );
    }

    bool BNTGPEvaluationCache::tryGet(
        const BNTGPEvaluationCacheKey key,
        const gp::GPTree& tree,
        BNTGPEvaluation& outputEvaluation) const
    {
        const auto bucketIterator = entries_.find(key);

        if (bucketIterator == entries_.end())
        {
            return false;
        }

        for (const CacheEntry& entry : bucketIterator->second)
        {
            if (!areBNTGPTreesIdentical(entry.tree, tree))
            {
                continue;
            }

            outputEvaluation = entry.evaluation;
            return true;
        }

        return false;
    }

    void BNTGPEvaluationCache::store(
        const BNTGPEvaluationCacheKey key,
        const gp::GPTree& tree,
        const BNTGPEvaluation& evaluation)
    {
        CacheBucket& bucket = entries_[key];

        for (CacheEntry& entry : bucket)
        {
            if (!areBNTGPTreesIdentical(entry.tree, tree))
            {
                continue;
            }

            entry.evaluation = evaluation;
            return;
        }

        bucket.push_back({ tree, evaluation });
        ++entryCount_;
    }

    void BNTGPEvaluationCache::clear() noexcept
    {
        entries_.clear();
        entryCount_ = 0U;
    }

    bool BNTGPEvaluationCache::empty() const noexcept
    {
        return entryCount_ == 0U;
    }

    std::size_t BNTGPEvaluationCache::size() const noexcept
    {
        return entryCount_;
    }
}
