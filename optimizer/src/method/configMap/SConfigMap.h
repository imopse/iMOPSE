#pragma once

#include <map>
#include <string>

struct SConfigMap
{
public:
    bool AddLine(const std::string &keyString, const std::string &valueString);
    bool HasValue(const std::string &paramKey);

    bool TakeValue(const std::string &paramKey, int &outValue);
    bool TakeValue(const std::string &paramKey, size_t &outValue);
    bool TakeValue(const std::string &paramKey, float &outValue);
    bool TakeValue(const std::string &paramKey, double &outValue);
    bool TakeValue(const std::string &paramKey, std::string &outValue);

    void Clear();
private:
    std::map<std::string, std::string> m_ConfigMap;
};