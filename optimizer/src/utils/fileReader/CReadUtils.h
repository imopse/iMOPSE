#pragma once

#include <string>
#include <vector>
#include <regex>

class CReadUtils
{
public:
    static bool fileExists(const char *path);
    static bool ReadKeyValueString(const std::string &line, const std::string &delimiter, std::string &keyString, std::string &valueString);

    static bool GotoReadSizeByKey(std::ifstream &fileStream, const std::string &lineKey, const std::string &delimiter, size_t &val);
    static bool GotoReadIntegerByKey(std::ifstream &fileStream, const std::string &lineKey, const std::string &delimiter, int &val);
    static bool GotoReadFloatByKey(std::ifstream &fileStream, const std::string &lineKey, const std::string &delimiter, float &val);
    static bool GotoReadStringByKey(std::ifstream &fileStream, const std::string &lineKey, const std::string &delimiter, std::string &val);
    static bool GoToReadFloatByKeyAndRegex(std::ifstream& fileStream, const std::string& lineKey, const std::regex& regex, float &val);

    static bool GotoLineByKey(std::ifstream &fileStream, const std::string &lineKey, std::string &line);

    static bool ReadSizeByKey(const std::string &line, const std::string &lineKey, const std::string &delimiter, size_t &val);
    static bool ReadIntegerByKey(const std::string &line, const std::string &lineKey, const std::string &delimiter, int &val);
    static bool ReadFloatByKey(const std::string &line, const std::string &lineKey, const std::string &delimiter, float &val);

    static bool ReadStringByKey(const std::string &line, const std::string &lineKey, const std::string &delimiter, std::string &val);


    static void ReadWeights(const std::string &rawWeightsString, std::vector<float> &objectiveWeights);

    static std::vector<std::string> SplitLine(const std::string& lineToSplit);
};