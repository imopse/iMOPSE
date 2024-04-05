#pragma once

#include <vector>
#include <cmath>

class SGenotype
{
public:
    SGenotype() = default;

    SGenotype(const SGenotype &other) : m_FloatGenotype(other.m_FloatGenotype), m_IntGenotype(other.m_IntGenotype),
                                        m_BoolGenotype(other.m_BoolGenotype)
    {};

    // TODO - why three different genotypes?
    std::vector<float> m_FloatGenotype;
    std::vector<int> m_IntGenotype;
    std::vector<bool> m_BoolGenotype;
};
