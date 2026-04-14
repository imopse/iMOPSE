#pragma once

#include <cstddef>
#include <vector>
#include <set>

struct SEncodingDescriptor
{
    float m_MinValue;
    float m_MaxValue;
};

enum class EEncodingType
{
    PERMUTATION,
    BINARY,
    ASSOCIATION
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
    std::set<EEncodingType> allowedEncodingTypes;

    std::set<EEncodingType> GetAllEncodingTypes()
    {
        std::set<EEncodingType> uniqueTypes;

        for (const auto &section: m_Encoding)
        {
            uniqueTypes.insert(section.m_SectionType);
        }

        return uniqueTypes;
    }
};