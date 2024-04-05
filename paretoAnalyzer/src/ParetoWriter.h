#pragma once
#include "ParetoFront.h"

class ParetoWriter
{
public:

	void WriteParetoToCSV(const char* directoryPath, const std::string& fileName, const ParetoFront& paretoToWrite);
};