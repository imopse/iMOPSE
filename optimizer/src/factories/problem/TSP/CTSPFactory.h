#pragma once

#include "../../../utils/fileReader/CReadUtils.h"
#include "../../../problem/problems/TSP/CTSP.h"
#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>

class CTSPFactory {
private:
    static const std::string s_Delimiter;
    static const std::string s_DimensionKey;
    static const std::string s_CitiesSectionKey;

    static CTSPTemplate *tspTemplate;

    static void ReadCities(std::ifstream &fileStream, int dimension, std::vector<CCity> &cities) {
        std::string line;
        if (!CReadUtils::GotoLineByKey(fileStream, s_CitiesSectionKey, line)) {
            throw std::runtime_error("Error reading cities for TSP");
        }

        for (int i = 0; i < dimension; ++i) {
            if (std::getline(fileStream, line)) {
                const std::vector<std::string> vec = CReadUtils::SplitLine(line);
                cities.emplace_back(std::stoi(vec[0]), std::stof(vec[1]), std::stof(vec[2]));
            }
        }
    }

public:
    static CTSPTemplate* ReadCTSPTemplate(const char *problemDefinitionPath) {
        auto *result = new CTSPTemplate();

        std::ifstream readFileStream(problemDefinitionPath);
        int dim = 0;
        if (!CReadUtils::GotoReadIntegerByKey(readFileStream, s_DimensionKey, s_Delimiter, dim)) {
            throw std::runtime_error("Error reading dimension for TSP");
        }

        std::vector<CCity> cities;
        ReadCities(readFileStream, dim, cities);
        result->SetCities(cities);

        readFileStream.close();
        return result;
    }
    
    static CTSP* CreateTSP(const char *problemDefinitionPath) {
        tspTemplate = ReadCTSPTemplate(problemDefinitionPath);
        tspTemplate->CalculateMaxDistance();
        return new CTSP(*tspTemplate);
    }

    static void DeleteObjects() {
        delete tspTemplate;
    }
};
