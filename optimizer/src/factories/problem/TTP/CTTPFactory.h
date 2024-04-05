#pragma once

#include <fstream>
#include <vector>
#include "../../../problem/problems/TTP/CTTP1.h"
#include "../../../problem/problems/TTP/CTTP2.h"
#include "../../../problem/problems/TTP/CTTPTemplate.h"

class CTTPFactory
{
public:
    static CTTP1 *CreateTTP1(const char *problemDefinitionPath);
    static CTTP2 *CreateTTP2(const char *problemDefinitionPath);
    static void DeleteObjects();
private:
    static const std::string s_Delimiter;
    static const std::string s_DimensionKey;
    static const std::string s_ItemsCountKey;
    static const std::string s_CapacityKey;
    static const std::string s_MinSpeedKey;
    static const std::string s_MaxSpeedKey;
    static const std::string s_RentingRatioKey;
    static const std::string s_CitiesSectionKey;
    static const std::string s_ItemsSectionKey;

    static CTTPTemplate *ttpTemplate;
    static CTTPTemplate *ReadTTPTemplate(const char *problemDefinitionPath);
    static void ReadCities(std::ifstream &fileStream, int dimension, std::vector<SCity> &cities);
    static void ReadItems(std::ifstream &fileStream, int itemCount, std::vector<SItem> &items);
};
