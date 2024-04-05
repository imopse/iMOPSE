#include "CTTPFactory.h"
#include "utils/fileReader/CReadUtils.h"
#include <string>
#include <vector>

#define READ_TTP 1

const std::string CTTPFactory::s_Delimiter = "\t";
const std::string CTTPFactory::s_DimensionKey = "DIMENSION:";
const std::string CTTPFactory::s_ItemsCountKey = "NUMBER OF ITEMS:";
const std::string CTTPFactory::s_CapacityKey = "CAPACITY OF KNAPSACK:";
const std::string CTTPFactory::s_MinSpeedKey = "MIN SPEED:";
const std::string CTTPFactory::s_MaxSpeedKey = "MAX SPEED:";
const std::string CTTPFactory::s_RentingRatioKey = "RENTING RATIO:";
const std::string CTTPFactory::s_CitiesSectionKey = "NODE_COORD_SECTION	(INDEX, X, Y):";
const std::string CTTPFactory::s_ItemsSectionKey = "ITEMS SECTION	(INDEX, PROFIT, WEIGHT, ASSIGNED NODE NUMBER):";

CTTPTemplate *CTTPFactory::ttpTemplate = nullptr;

CTTP1 *CTTPFactory::CreateTTP1(const char *problemDefinitionPath)
{
    CTTP2 *ttp2 = CreateTTP2(problemDefinitionPath);
    return new CTTP1(*ttp2);
}

CTTP2 *CTTPFactory::CreateTTP2(const char *problemDefinitionPath)
{
    ttpTemplate = ReadTTPTemplate(problemDefinitionPath);
    return new CTTP2(*ttpTemplate);
}

void CTTPFactory::DeleteObjects()
{
    delete ttpTemplate;
}

CTTPTemplate *CTTPFactory::ReadTTPTemplate(const char *problemDefinitionPath)
{
    auto *result = new CTTPTemplate();

    std::ifstream readFileStream(problemDefinitionPath);

    int dim = 0;
    if (!CReadUtils::GotoReadIntegerByKey(readFileStream, s_DimensionKey, s_Delimiter, dim))
    {
        throw std::runtime_error("Error reading dim (dimensions) for TTP");
    }
#if READ_TTP
    int itemCount = 0;
    if (!CReadUtils::GotoReadIntegerByKey(readFileStream, s_ItemsCountKey, s_Delimiter, itemCount))
    {
        throw std::runtime_error("Error reading itemCount for TTP");
    }
    int capacity = 0;
    if (!CReadUtils::GotoReadIntegerByKey(readFileStream, s_CapacityKey, s_Delimiter, capacity))
    {
        throw std::runtime_error("Error reading capacity for TTP");
    }

    float minSpeed = 0.f;
    if (!CReadUtils::GotoReadFloatByKey(readFileStream, s_MinSpeedKey, s_Delimiter, minSpeed))
    {
        throw std::runtime_error("Error reading minSpeed for TTP");
    }
    float maxSpeed = 0.f;
    if (!CReadUtils::GotoReadFloatByKey(readFileStream, s_MaxSpeedKey, s_Delimiter, maxSpeed))
    {
        throw std::runtime_error("Error reading maxSpeed for TTP");
    }
    float rentRatio = 0.f;
    if (!CReadUtils::GotoReadFloatByKey(readFileStream, s_RentingRatioKey, s_Delimiter, rentRatio))
    {
        throw std::runtime_error("Error reading rentRatio for TTP");
    }
#endif

    std::vector<SCity> cities;
    ReadCities(readFileStream, dim, cities);

    std::vector<SItem> items;
#if READ_TTP
    ReadItems(readFileStream, itemCount, items);
#endif

#if USE_EOK
    // Add extra EOK flag item
    items.emplace_back(0, 0, 0, 0);
#endif

    std::string pathString(problemDefinitionPath);
    size_t fileNameStartPos = pathString.rfind('/') + 1;
    size_t fileNameEndPos = pathString.rfind('.');
    result->SetFileName(pathString.substr(fileNameStartPos, fileNameEndPos - fileNameStartPos));
#if READ_TTP
    result->SetData(cities, items, capacity, minSpeed, maxSpeed, rentRatio);
#else
    ttp.SetData(cities, items, 0, 0, 1, 1);
#endif

    readFileStream.close();

    return result;
}

void CTTPFactory::ReadCities(std::ifstream &fileStream, int dimension, std::vector<SCity> &cities)
{
    std::string line;
    if (!CReadUtils::GotoLineByKey(fileStream, s_CitiesSectionKey, line))
    {
        throw std::runtime_error("Error reading cities for TTP");
    }

    for (int i = 0; i < dimension; ++i)
    {
        if (std::getline(fileStream, line))
        {
            const std::vector<std::string> vec = CReadUtils::SplitLine(line);
            cities.emplace_back(std::stoi(vec[0]), std::stof(vec[1]), std::stof(vec[2]));
        }
    }
}

void CTTPFactory::ReadItems(std::ifstream &fileStream, int itemCount, std::vector<SItem> &items)
{
    std::string line;
    if (!CReadUtils::GotoLineByKey(fileStream, s_ItemsSectionKey, line))
    {
        throw std::runtime_error("Error reading items for TTP");
    }

    for (int i = 0; i < itemCount; ++i)
    {
        if (std::getline(fileStream, line))
        {
            const std::vector<std::string> vec = CReadUtils::SplitLine(line);
            items.emplace_back(std::stoi(vec[0]), std::stoi(vec[1]), std::stoi(vec[2]), std::stoi(vec[3]));
        }
    }
}
