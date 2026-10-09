#pragma once

#include "../core/BNTGPIndividual.hpp"
#include "BNTGPTreeIdentity.hpp"

#include <cstddef>
#include <unordered_map>
#include <vector>

namespace bntgp
{
    class BNTGPEvaluationCache final
    {
    public:
        void resetForRun(std::size_t populationSize);

        [[nodiscard]]
        bool tryGet(
            BNTGPEvaluationCacheKey key,
            const gp::GPTree& tree,
            BNTGPEvaluation& outputEvaluation
        ) const;

        void store(
            BNTGPEvaluationCacheKey key,
            const gp::GPTree& tree,
            const BNTGPEvaluation& evaluation
        );

        void clear() noexcept;

        [[nodiscard]]
        bool empty() const noexcept;

        [[nodiscard]]
        std::size_t size() const noexcept;

    private:
        struct CacheEntry final
        {
            gp::GPTree tree{};
            BNTGPEvaluation evaluation{};
        };

        using CacheBucket = std::vector<CacheEntry>;
        using CacheMap = std::unordered_map<
            BNTGPEvaluationCacheKey,
            CacheBucket
        >;

        CacheMap entries_{};
        std::size_t entryCount_{0U};
    };
}
