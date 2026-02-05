#pragma once
#include "gp/GPTree.hpp"
#include "gp/Features.hpp"
#include "domain/Instance.hpp"
#include "domain/Task.hpp"
#include "domain/Resource.hpp"

class GPTreeResRule {
public:
    const GPTree* tree = nullptr;

    GPTreeResRule() = default;
    explicit GPTreeResRule(const GPTree& t) : tree(&t) {}

    double score(const Instance& I, const Task& t, const Resource& r, int now) const {
        if (!tree) return 0.0;
        Features f = computeResourceFeatures(I, t, r, now);
        return tree->eval(f);
    }
};
