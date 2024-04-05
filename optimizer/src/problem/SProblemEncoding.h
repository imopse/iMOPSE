#pragma once

#include <cstddef>
#include <vector>

struct SEncodingDescriptor
{
    float m_MinValue;
    float m_MaxValue;
};

enum class EEncodingType
{
    ASSOCIATION = 0,
    PERMUTATION,
    BINARY,
};

struct SEncodingSection
{
    std::vector<SEncodingDescriptor> m_SectionDescription;
    EEncodingType m_SectionType;
};

struct SProblemEncoding
{
    int m_objectivesNumber;
    std::vector<SEncodingSection> m_Encoding;
    std::vector<std::vector<float>> m_additionalProblemData;
};