#include "CECVRPTWFactory.h"
#include "../../../utils/fileReader/CReadUtils.h"
#include <regex>
#include <iostream>

#define READ_ECVRPTW 1

const std::string CECVRPTWFactory::s_Delimiter = ": ";
const std::string CECVRPTWFactory::s_CapacityKey = "C";
const std::string CECVRPTWFactory::s_TankCapacityKey = "Q";
const std::string CECVRPTWFactory::s_FuelConsumptionKey = "r";
const std::string CECVRPTWFactory::s_InverseRefuelingRateKey = "g";
const std::string CECVRPTWFactory::s_VelocityKey = "v";
const std::string CECVRPTWFactory::s_VehicleCountKey = "n";
const std::string CECVRPTWFactory::s_CitiesSectionKey = "StringID";

CECVRPTWTemplate* CECVRPTWFactory::cvrpTemplate = nullptr;

CECVRPTW* CECVRPTWFactory::CreateECVRPTW(const char* problemDefinitionPath) {
    cvrpTemplate = ReadECVRPTWTemplate(problemDefinitionPath);
    return new CECVRPTW(*cvrpTemplate);
}

void CECVRPTWFactory::DeleteObjects() {
    delete cvrpTemplate;
}

CECVRPTWTemplate* CECVRPTWFactory::ReadECVRPTWTemplate(const char* problemDefinitionPath) {
    auto* result = new CECVRPTWTemplate();

    std::ifstream readFileStream(problemDefinitionPath);

    /*std::string test;
    std::cin >> test;*/

    int dimension = 0;
    std::vector<SCityECVRPTW>* cities = new std::vector<SCityECVRPTW>();
    ReadCities(readFileStream, dimension, *cities);

    auto const regex = std::regex{ "[0-9]+[.][0-9]+" };

    float fuelTankCapacity = 0;
    if (!CReadUtils::GoToReadFloatByKeyAndRegex(readFileStream, s_TankCapacityKey, regex, fuelTankCapacity))
        throw std::runtime_error("Error reading tank capacity for CVRP");

    float capacity = 0;
    if (!CReadUtils::GoToReadFloatByKeyAndRegex(readFileStream, s_CapacityKey, regex, capacity))
        throw std::runtime_error("Error reading capacity for CVRP");

    float fuelConsumptionRate = 0;
    if (!CReadUtils::GoToReadFloatByKeyAndRegex(readFileStream, s_FuelConsumptionKey, regex, fuelConsumptionRate))
        throw std::runtime_error("Error reading fuel consumption rate for CVRP");

    float inverseRefuelingRate = 0;
    if (!CReadUtils::GoToReadFloatByKeyAndRegex(readFileStream, s_InverseRefuelingRateKey, regex, inverseRefuelingRate))
        throw std::runtime_error("Error reading refueling rate for CVRP");

    float velocity = 0;
    if (!CReadUtils::GoToReadFloatByKeyAndRegex(readFileStream, s_VelocityKey, regex, velocity))
        throw std::runtime_error("Error reading velocity for CVRP");

    float vehicleCount = 0;
    if (!CReadUtils::GoToReadFloatByKeyAndRegex(readFileStream, s_VehicleCountKey, regex, vehicleCount))
        throw std::runtime_error("Error reading velocity for CVRP");

    std::vector<size_t>* depotIndexes = new std::vector<size_t>();
    std::vector<size_t>* chargingStationIndexes = new std::vector<size_t>();
    std::vector<size_t>* customerIndexes = new std::vector<size_t>();

    for (int i = 0; i < cities->size(); i++) {
        switch ((*cities)[i].m_type) {
            case ENodeType::Depot:
                depotIndexes->emplace_back(i);
                chargingStationIndexes->emplace_back(i);
                break;
            case ENodeType::ChargingStation:
                chargingStationIndexes->emplace_back(i);
                break;
            case ENodeType::Customer:
                customerIndexes->emplace_back(i);
                break;
        }
    }

    int trucks = 1;
    std::string pathString(problemDefinitionPath);
    size_t fileNameStartPos = pathString.rfind("/") + 1;
    size_t fileNameEndPos = pathString.rfind(".");
    result->SetFileName(pathString.substr(fileNameStartPos, fileNameEndPos - fileNameStartPos));
#if READ_ECVRPTW
    result->SetData(*cities,
        capacity,
        trucks,
        fuelTankCapacity,
        fuelConsumptionRate,
        inverseRefuelingRate,
        velocity,
        *chargingStationIndexes,
        *depotIndexes,
        *customerIndexes,
        vehicleCount
    );
#endif

    readFileStream.close();
    return result;
}

void CECVRPTWFactory::ReadCities(std::ifstream& fileStream, int& dimension, std::vector<SCityECVRPTW>& cities) {
    std::string line;

    if (CReadUtils::GotoLineByKey(fileStream, s_CitiesSectionKey, line)) {
        dimension = 0;
        while (std::getline(fileStream, line) && !line.empty()) {
            auto const re = std::regex{ R"(\s+)" };
            auto const vec = std::vector<std::string>(std::sregex_token_iterator{ line.begin(), line.end(), re, -1 }, std::sregex_token_iterator{});
            cities.emplace_back(dimension++,
                vec[0],
                (ENodeType)vec[1][0],
                std::stof(vec[2]),
                std::stof(vec[3]),
                std::stof(vec[4]),
                std::stof(vec[5]),
                std::stof(vec[6]),
                std::stof(vec[7])

            );
        }
    }
    else {
        throw std::runtime_error("Error reading cities for ECVRPTW");
    }
}