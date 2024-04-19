#pragma once

#include "../../../problem/problems/ECVRPTW/CECVRPTW.h"
#include <fstream>
#include <vector>

class CECVRPTWFactory {
public:
    static CECVRPTW* CreateECVRPTW(const char* problemDefinitionPath);
    static void DeleteObjects();
private:
    static const std::string s_Delimiter;
    static const std::string s_CapacityKey;
    static const std::string s_CitiesSectionKey;
    static const std::string s_TankCapacityKey;
    static const std::string s_FuelConsumptionKey;
    static const std::string s_InverseRefuelingRateKey;
    static const std::string s_VelocityKey;
    static const std::string s_VehicleCountKey;

    static CECVRPTWTemplate* cvrpTemplate;
    static CECVRPTWTemplate* ReadECVRPTWTemplate(const char* problemDefinitionPath);
    static void ReadCities(std::ifstream& fileStream, int& dimension, std::vector<SCityECVRPTW>& cities);
};