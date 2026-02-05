#pragma once
#include <string>
#include "domain/Task.hpp"

class Instance;

class IDispatchingRule {
public:
    virtual ~IDispatchingRule() = default;

    virtual double score(const Task& t) const = 0;
    virtual std::string name() const = 0;

    virtual void setContext(const Instance* /*inst*/, int /*now*/) {}
};
