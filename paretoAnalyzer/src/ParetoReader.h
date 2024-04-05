#pragma once
#include "ConfigResults.h"

class ParetoReader
{
public:

	void ReadConfigParetos(const char* directoryPath, ConfigResults& configResults, const char* filterInstanceName = nullptr);
	void ReadTaguchiParetos(const char* directoryPath, std::vector<ConfigData>& configsToAnalyze);
	void ReadNTGA2Paretos(const char* filePath, ConfigResults& configResults);

	void ReadParetoFromCSV(const char* directoryPath, const std::string& fileName, ParetoFront& paretoToRead);
};