#include "ParetoReader.h"
#include "ParetoWriter.h"
#include "ConfigResults.h"
#include "InstancesOrder.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cmath>

int main(int argc, char* argv[]) {
    std::vector<ConfigData*> configsToAnalyze;
    
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <configurationFilePath> <instanceName> <outputDir>" << std::endl;
        return 1;
    }

    const char* configurationFilePath = argv[1];
    const char* instanceName = argv[2];
    const char* outputDir = argv[3];

    std::ifstream configFile(configurationFilePath);
    if (!configFile.is_open()) {
        std::cerr << "Failed to open configuration file: " << configurationFilePath << std::endl;
        return 1;
    }

    ParetoReader paretoReader;
    std::string line;
    while (std::getline(configFile, line)) {
        std::filesystem::path dirPath(line);
        std::string name = dirPath.filename().string();

        auto* newConfig = new ConfigData(name, line);
        configsToAnalyze.emplace_back(newConfig);
        paretoReader.ReadConfigParetos(newConfig->configPath.c_str(), newConfig->configResults, instanceName);
    }

    configFile.close();
    
    // Analyze instance
    std::cout << "--- Instance " << instanceName << " ---" << std::endl;

    bool allContains = true;
    for (const ConfigData* config : configsToAnalyze)
    {
        if (!config->configResults.Contains(instanceName))
        {
            allContains = false;
            std::cout << "Missing: " << instanceName << " results for config: " << config->configName << std::endl;
            break;
        }
    }

    if (!allContains)
    {
        return 0;
    }

    ParetoFront trueParetoFront;

    ParetoWriter paretoWriter;
    // Calculate true Pareto and write
    for (ConfigData* config : configsToAnalyze)
    {
        config->configMergedPareto = config->configResults.GetMergedParetoFronts(instanceName);
        paretoWriter.WriteParetoToCSV(outputDir, config->configName + "_merged", config->configMergedPareto);
        trueParetoFront = trueParetoFront.Merge(config->configMergedPareto);
    }
    paretoWriter.WriteParetoToCSV(outputDir,  "true_pareto_front_approximation", trueParetoFront);

    size_t trueParetoFrontSize = trueParetoFront.solutions.size();
    std::cout << "TPFS:" << trueParetoFrontSize << std::endl;

    if (trueParetoFrontSize == 0)
    {
        std::cout << "TPFS == 0, shutting down!" << std::endl;
        return 0;
    }

    // Normalize points
    {
        size_t sol1Size = trueParetoFront.solutions[0].size();

        // Initialize Min and Max values
        std::vector<float> minValues(sol1Size, 0.f);
        std::vector<float> maxValues(sol1Size, 0.f);
        for (size_t v = 0; v < sol1Size; ++v)
        {
            maxValues[v] = minValues[v] = trueParetoFront.solutions[0][v];
        }

        for (size_t i = 0; i < configsToAnalyze.size(); ++i) 
        {
            // Find Min and Max in TrueParetoFront
            for (size_t j = 0; j < configsToAnalyze[i]->configResults.m_ConfigParetos[instanceName].size(); ++j)
            {
                for (size_t k = 0; k < configsToAnalyze[i]->configResults.m_ConfigParetos[instanceName][j].solutions.size(); k++) {
                    const std::vector<float>& sol = configsToAnalyze[i]->configResults.m_ConfigParetos[instanceName][j].solutions[k];
                    for (size_t v = 0; v < sol1Size; ++v)
                    {
                        float evalValue = sol[v];
                        minValues[v] = fminf(minValues[v], evalValue);
                        maxValues[v] = fmaxf(maxValues[v], evalValue);
                    }
                }
            }
        }
        

        // Update Pareto data
        if (!trueParetoFront.NormalizeByMinMax(minValues, maxValues))
        {
            std::cout << "Error while trying to normalize TrueParetoFront!" << std::endl;
        }
        // Update config data
        for (ConfigData* config : configsToAnalyze)
        {
            if (!config->configResults.NormalizeByMinMax(instanceName, minValues, maxValues))
            {
                std::cout << "Error while trying to normalize Config: " << config->configName << "!" << std::endl;
            }
            if (!config->configMergedPareto.NormalizeByMinMax(minValues, maxValues))
            {
                std::cout << "Error while trying to normalize ConfigMergedPareto: " << config->configName << "!" << std::endl;
            }
        }
    }
    
    {
        for (ConfigData* config : configsToAnalyze)
        {
            //ParetoMetrics avgParetoMetrics = config.configResults.EvaluateByTrueParetoFront_IGD(selInstName, trueParetoFront);
            ParetoMetrics avgParetoMetrics = config->configResults.EvaluateByTrueParetoFront(instanceName, trueParetoFront);
            //config.configResultsDump.push_back(selInstName + ";" + avgParetoMetrics.ToString());
            size_t runsCount = config->configResults.GetParetoCountForInstance(instanceName);
            size_t mergedSize = config->configMergedPareto.solutions.size();
            size_t mergedNonDominated = config->configMergedPareto.GetNumberOfNonDominatedBy(trueParetoFront);
            std::cout << config->configName << ";runs:" << runsCount << ";MPFS:" << mergedSize << ";MND:" << mergedNonDominated << ";" << avgParetoMetrics.ToString() << std::endl;
        }
    }
	return 0;
}