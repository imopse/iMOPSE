#include "CCVRPFactory.h"
#include "utils/fileReader/CReadUtils.h"
#include <regex>

#define READ_CVRP 1

const std::string CCVRPFactory::s_Delimiter = ": ";
const std::string CCVRPFactory::s_DimensionKey = "DIMENSION :";
const std::string CCVRPFactory::s_CapacityKey = "CAPACITY :";
const std::string CCVRPFactory::s_CitiesSectionKey = "NODE_COORD_SECTION";
const std::string CCVRPFactory::s_DemandSectionKey = "DEMAND_SECTION";
const std::string CCVRPFactory::s_DepotSectionKey = "DEPOT_SECTION";

CCVRPTemplate *CCVRPFactory::cvrpTemplate = nullptr;

CCVRP *CCVRPFactory::CreateCVRP(const char *problemDefinitionPath) {
    cvrpTemplate = ReadCVRPTemplate(problemDefinitionPath);
    return new CCVRP(*cvrpTemplate);
}

void CCVRPFactory::DeleteObjects() {
    delete cvrpTemplate;
}

CCVRPTemplate *CCVRPFactory::ReadCVRPTemplate(const char *problemDefinitionPath) {
    auto *result = new CCVRPTemplate();

    std::ifstream readFileStream(problemDefinitionPath);

    int dimension = 0;
    if (!CReadUtils::GotoReadIntegerByKey(readFileStream, s_DimensionKey, s_Delimiter, dimension))
        throw std::runtime_error("Error reading dimension for CVRP");
#if READ_CVRP
    int capacity = 0;
    if (!CReadUtils::GotoReadIntegerByKey(readFileStream, s_CapacityKey, s_Delimiter, capacity))
        throw std::runtime_error("Error reading capacity for CVRP");
#endif

    std::vector<SCityCVRP> cities;
    ReadCitiesAndDemand(readFileStream, dimension, cities);

    std::vector<size_t> depotIndexes;
    ReadDepot(readFileStream, depotIndexes);

    int trucks = 1;
    std::string pathString(problemDefinitionPath);
    size_t fileNameStartPos = pathString.rfind("/") + 1;
    size_t fileNameEndPos = pathString.rfind(".");
    result->SetFileName(pathString.substr(fileNameStartPos, fileNameEndPos - fileNameStartPos));
#if READ_CVRP
    result->SetData(cities, capacity, trucks, depotIndexes);
#endif

    readFileStream.close();
    return result;
}

void CCVRPFactory::ReadCitiesAndDemand(std::ifstream &fileStream, int dimension, std::vector<SCityCVRP> &cities) {
    std::string line;
    std::vector<float> x;
    std::vector<float> y;
    std::vector<int> demand;

    if (CReadUtils::GotoLineByKey(fileStream, s_CitiesSectionKey, line)) {
        for (int i = 0; i < dimension; ++i) {
            if (std::getline(fileStream, line)) {
                auto const vec = CReadUtils::SplitLine(line);
                x.emplace_back(std::stof(vec[2]));
                y.emplace_back(std::stof(vec[3]));
            }
        }
    }

    if (CReadUtils::GotoLineByKey(fileStream, s_DemandSectionKey, line)) {
        for (int i = 0; i < dimension; ++i) {
            if (std::getline(fileStream, line)) {
                auto const vec = CReadUtils::SplitLine(line);
                cities.emplace_back(std::stoi(vec[0]), x[i], y[i], std::stoi(vec[1]));
            }
        }
    }
}

void CCVRPFactory::ReadDepot(std::ifstream &fileStream, std::vector<size_t> &depotIndexes) {
    std::string line;

    if (CReadUtils::GotoLineByKey(fileStream, s_DepotSectionKey, line)) {
        while(true) {
            if (std::getline(fileStream, line)) {
                int index = std::stoi(line);
                if (index == -1)
                    break;
                depotIndexes.emplace_back(index);
            }
        }
    }
}
