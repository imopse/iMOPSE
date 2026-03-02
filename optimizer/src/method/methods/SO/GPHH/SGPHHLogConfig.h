#pragma once

#include "../../../configMap/SConfigMap.h"

struct SGPHHLogConfig {
    bool EnableLogging = false;
    bool EnableConsoleOutput = true;
    int LogInterval = 1;
    bool EnableGenerationReport = false;
    bool EnableFinalReport = false;
    
    bool LogGenerationTime = false;
    bool LogBestFitness = false;
    bool LogAvgFitness = false;
    bool LogWorstFitness = false;
    bool LogStdDevFitness = false;
    
    bool LogMedianFitness = false;
    bool LogVarianceFitness = false;
    bool LogP90Fitness = false;
    bool LogP95Fitness = false;
    
    bool LogBestDistance = false;
    bool LogBestFormula = false;
    bool LogBestSolution = false;
    bool LogPopulationDetails = false;
    bool LogOptimalValue = false;
    
    bool LogIndividualFitness = false;

    static SGPHHLogConfig LoadConfig(SConfigMap* configMap) {
        SGPHHLogConfig config;
        int val;

        if (configMap->TakeValue("EnableLogging", val)) config.EnableLogging = (val != 0);
        if (configMap->TakeValue("EnableConsoleOutput", val)) config.EnableConsoleOutput = (val != 0);
        if (configMap->TakeValue("LogInterval", val)) config.LogInterval = val;
        if (configMap->TakeValue("EnableGenerationReport", val)) config.EnableGenerationReport = (val != 0);
        if (configMap->TakeValue("EnableFinalReport", val)) config.EnableFinalReport = (val != 0);

        if (configMap->TakeValue("LogGenerationTime", val)) config.LogGenerationTime = (val != 0);
        if (configMap->TakeValue("LogBestFitness", val)) config.LogBestFitness = (val != 0);
        if (configMap->TakeValue("LogAvgFitness", val)) config.LogAvgFitness = (val != 0);
        if (configMap->TakeValue("LogWorstFitness", val)) config.LogWorstFitness = (val != 0);
        if (configMap->TakeValue("LogStdDevFitness", val)) config.LogStdDevFitness = (val != 0);

        if (configMap->TakeValue("LogMedianFitness", val)) config.LogMedianFitness = (val != 0);
        if (configMap->TakeValue("LogVarianceFitness", val)) config.LogVarianceFitness = (val != 0);
        if (configMap->TakeValue("LogP90Fitness", val)) config.LogP90Fitness = (val != 0);
        if (configMap->TakeValue("LogP95Fitness", val)) config.LogP95Fitness = (val != 0);

        if (configMap->TakeValue("LogBestDistance", val)) config.LogBestDistance = (val != 0);
        if (configMap->TakeValue("LogBestFormula", val)) config.LogBestFormula = (val != 0);
        if (configMap->TakeValue("LogBestSolution", val)) config.LogBestSolution = (val != 0);
        if (configMap->TakeValue("LogPopulationDetails", val)) config.LogPopulationDetails = (val != 0);
        if (configMap->TakeValue("LogOptimalValue", val)) config.LogOptimalValue = (val != 0);
        if (configMap->TakeValue("LogIndividualFitness", val)) config.LogIndividualFitness = (val != 0);

        return config;
    }
};
