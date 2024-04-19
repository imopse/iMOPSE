#include "CReadUtils.h"
#include <fstream>
#include <sstream>
#include <sys/stat.h> // Library for file system statistics

bool CReadUtils::fileExists(const char *path)
{
    struct stat buffer{}; // Create a stat structure
    return (stat(path, &buffer) == 0); // Return true if the file exists
}

bool CReadUtils::ReadKeyValueString(const std::string &line, const std::string &delimiter, std::string &keyString,
                                    std::string &valueString)
{
    size_t delimiterPos = line.find(delimiter);
    if (delimiterPos > 0 && delimiterPos < line.length())
    {
        keyString = line.substr(0, delimiterPos);
        valueString = line.substr(delimiterPos + 1, line.length() - delimiterPos);
        return true;
    }
    return false;
}

bool CReadUtils::GotoReadSizeByKey(std::ifstream &fileStream, const std::string &lineKey, const std::string &delimiter,
                                   size_t &val)
{
    std::string line;
    if (CReadUtils::GotoLineByKey(fileStream, lineKey, line))
    {
        return CReadUtils::ReadSizeByKey(line, lineKey, delimiter, val);
    }
    return false;
}

bool
CReadUtils::GotoReadIntegerByKey(std::ifstream &fileStream, const std::string &lineKey, const std::string &delimiter,
                                 int &val)
{
    std::string line;
    if (CReadUtils::GotoLineByKey(fileStream, lineKey, line))
    {
        return CReadUtils::ReadIntegerByKey(line, lineKey, delimiter, val);
    }
    return false;
}

bool CReadUtils::GotoReadFloatByKey(std::ifstream &fileStream, const std::string &lineKey, const std::string &delimiter,
                                    float &val)
{
    std::string line;
    if (CReadUtils::GotoLineByKey(fileStream, lineKey, line))
    {
        return CReadUtils::ReadFloatByKey(line, lineKey, delimiter, val);
    }
    return false;
}

bool
CReadUtils::GotoReadStringByKey(std::ifstream &fileStream, const std::string &lineKey, const std::string &delimiter,
                                std::string &val)
{
    std::string line;
    if (CReadUtils::GotoLineByKey(fileStream, lineKey, line))
    {
        return CReadUtils::ReadStringByKey(line, lineKey, delimiter, val);
    }
    return false;
}

bool CReadUtils::GoToReadFloatByKeyAndRegex(std::ifstream& fileStream, const std::string& lineKey, const std::regex& regex, float& val)
{
    std::string line;
    if (CReadUtils::GotoLineByKey(fileStream, lineKey, line))
    {
        std::smatch match;
        if (std::regex_search(line, match, regex)) {
            val = std::stof(match[0]);
            return true;
        }
        return true;
    }
    return false;
}

bool CReadUtils::GotoLineByKey(std::ifstream &fileStream, const std::string &lineKey, std::string &line)
{
    while (std::getline(fileStream, line))
    {
        if (line.rfind(lineKey, 0) == 0)
        {
            return true;
        }
    }
    return false;
}

bool CReadUtils::ReadSizeByKey(const std::string &line, const std::string &lineKey, const std::string &delimiter,
                               size_t &val)
{
    int integerVal = 0;
    if (ReadIntegerByKey(line, lineKey, delimiter, integerVal))
    {
        if (integerVal >= 0)
        {
            val = (size_t) integerVal;
            return true;
        }
    }
    return false;
}

bool CReadUtils::ReadIntegerByKey(const std::string &line, const std::string &lineKey, const std::string &delimiter,
                                  int &val)
{
    if (line.rfind(lineKey, 0) == 0)
    {
        size_t delimiterPos = line.find(delimiter);
        if (delimiterPos >= 0)
        {
            std::string stringVal = line.substr(delimiterPos + 1, line.length() - delimiterPos);
            val = std::stoi(stringVal);
            return true;
        }
    }
    return false;
}

bool CReadUtils::ReadFloatByKey(const std::string &line, const std::string &lineKey, const std::string &delimiter,
                                float &val)
{
    if (line.rfind(lineKey, 0) == 0)
    {
        size_t delimiterPos = line.find(delimiter);
        if (delimiterPos >= 0)
        {
            std::string stringVal = line.substr(delimiterPos + 1, line.length() - delimiterPos);
            val = std::stof(stringVal);
            return true;
        }
    }
    return false;
}

bool CReadUtils::ReadStringByKey(const std::string &line, const std::string &lineKey, const std::string &delimiter,
                                 std::string &val)
{
    if (line.rfind(lineKey, 0) == 0)
    {
        size_t delimiterPos = line.find(delimiter);
        if (delimiterPos >= 0)
        {
            val = line.substr(delimiterPos + 1, line.length() - delimiterPos);
            return true;
        }
    }
    return false;
}

void CReadUtils::ReadWeights(const std::string &rawWeightsString, std::vector<float> &objectiveWeights)
{
    std::istringstream stream(rawWeightsString.substr(1, rawWeightsString.length() - 2));
    std::string token;
    std::vector<float> values;

    while (std::getline(stream, token, ','))
    {
        values.push_back(std::stof(token));  // convert string to float
    }

    for (float &value: values)
    {
        objectiveWeights.emplace_back(value);
    }
}

std::vector<std::string> CReadUtils::SplitLine(const std::string& lineToSplit)
{
    auto const re = std::regex{R"(\s+)"};
    auto const vec = std::vector<std::string>(
        std::sregex_token_iterator{
            lineToSplit.begin(),
            lineToSplit.end(),
            re, -1
        },
        std::sregex_token_iterator{}
    );
    return vec;
}
