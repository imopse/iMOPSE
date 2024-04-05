#pragma once

#include "../../../problem/problems/CVRP/CCVRP.h"
#include <fstream>
#include <vector>

class CCVRPFactory {
public:
    static CCVRP *CreateCVRP(const char *problemDefinitionPath);
    static void DeleteObjects();
private:
    static const std::string s_Delimiter;
    static const std::string s_DimensionKey;
    static const std::string s_CapacityKey;
    static const std::string s_CitiesSectionKey;
    static const std::string s_DemandSectionKey;
    static const std::string s_DepotSectionKey;

    static CCVRPTemplate *cvrpTemplate;
    static CCVRPTemplate *ReadCVRPTemplate(const char *problemDefinitionPath);
    static void ReadCitiesAndDemand(std::ifstream& fileStream, int dimension, std::vector<SCityCVRP>& cities) ;
    static void ReadDepot(std::ifstream& fileStream, std::vector<size_t>& depotIndexes) ;
};