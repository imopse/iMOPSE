#include "CECVRPTWFactory.h"
#include "utils/fileReader/CReadUtils.h"
#include <regex>
#include <iostream>


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

    int dimension = 0;
    std::vector<SCityECVRPTW> cities;
    ReadCities(readFileStream, dimension, cities);

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

    readFileStream.close();

    std::vector<size_t> depotIndexes;
    std::vector<size_t> chargingStationIndexes;
    std::vector<size_t> customerIndexes;

    for (int i = 0; i < cities.size(); i++)
    {
        switch (cities[i].m_Type)
        {
            case ENodeType::Depot:
                depotIndexes.emplace_back(i);
                chargingStationIndexes.emplace_back(i);
                break;
            case ENodeType::ChargingStation:
                chargingStationIndexes.emplace_back(i);
                break;
            case ENodeType::Customer:
                customerIndexes.emplace_back(i);
                break;
        }
    }

    std::string pathString(problemDefinitionPath);
    size_t fileNameStartPos = pathString.rfind("/") + 1;
    size_t fileNameEndPos = pathString.rfind(".");
    result->SetFileName(pathString.substr(fileNameStartPos, fileNameEndPos - fileNameStartPos));
    result->SetData(cities,
                    capacity,
                    fuelTankCapacity,
                    fuelConsumptionRate,
                    inverseRefuelingRate,
                    velocity,
                    chargingStationIndexes,
                    depotIndexes,
                    customerIndexes,
                    (int)vehicleCount
    );

    if (!result->Validate())
    {
        throw std::runtime_error("Instance is invalid: " + std::string(problemDefinitionPath));
    }

    return result;
}

void CECVRPTWFactory::ReadCities(std::ifstream& fileStream, int& dimension, std::vector<SCityECVRPTW>& cities) {
    std::string line;

    if (CReadUtils::GotoLineByKey(fileStream, s_CitiesSectionKey, line))
    {
        dimension = 0;
        while (std::getline(fileStream, line) && !line.empty())
        {
            auto const re = std::regex{ R"(\s+)" };
            auto const vec = std::vector<std::string>(std::sregex_token_iterator{ line.begin(), line.end(), re, -1 }, std::sregex_token_iterator{});
            cities.emplace_back(dimension++,
                                vec[0],
                                (ENodeType)vec[1][0],
                                std::stof(vec[2]),
                                std::stof(vec[3]),
                                (int)std::stof(vec[4]),
                                std::stof(vec[5]),
                                std::stof(vec[6]),
                                std::stof(vec[7])
            );
        }
    }
    else
    {
        throw std::runtime_error("Error reading cities for ECVRPTW");
    }
}