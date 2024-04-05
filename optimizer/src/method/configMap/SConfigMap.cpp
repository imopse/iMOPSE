

#include "SConfigMap.h"

bool SConfigMap::AddLine(const std::string &keyString, const std::string &valueString)
{
    if (m_ConfigMap.find(keyString) != m_ConfigMap.end())
    {
        return false;
    }
    m_ConfigMap.insert(std::pair<std::string, std::string>(keyString, valueString));
    return true;
}

bool SConfigMap::HasValue(const std::string &paramKey)
{
    const auto &it = m_ConfigMap.find(paramKey);
    return it != m_ConfigMap.end();
}

bool SConfigMap::TakeValue(const std::string &paramKey, int &outValue)
{
    const auto &it = m_ConfigMap.find(paramKey);
    if (it != m_ConfigMap.end())
    {
        outValue = std::stoi(it->second);
        m_ConfigMap.erase(it);
        return true;
    }
    return false;
}

bool SConfigMap::TakeValue(const std::string &paramKey, size_t &outValue)
{
    const auto &it = m_ConfigMap.find(paramKey);
    if (it != m_ConfigMap.end())
    {
        outValue = std::stoull(it->second);
        m_ConfigMap.erase(it);
        return true;
    }
    return false;
}

bool SConfigMap::TakeValue(const std::string &paramKey, float &outValue)
{
    const auto &it = m_ConfigMap.find(paramKey);
    if (it != m_ConfigMap.end())
    {
        outValue = std::stof(it->second);
        m_ConfigMap.erase(it);
        return true;
    }
    return false;
}

bool SConfigMap::TakeValue(const std::string &paramKey, double &outValue)
{
    const auto &it = m_ConfigMap.find(paramKey);
    if (it != m_ConfigMap.end())
    {
        outValue = std::stod(it->second);
        m_ConfigMap.erase(it);
        return true;
    }
    return false;
}

bool SConfigMap::TakeValue(const std::string &paramKey, std::string &outValue)
{
    const auto &it = m_ConfigMap.find(paramKey);
    if (it != m_ConfigMap.end())
    {
        outValue = it->second;
        m_ConfigMap.erase(it);
        return true;
    }
    return false;
}

void SConfigMap::Clear()
{
    m_ConfigMap.clear();
}
