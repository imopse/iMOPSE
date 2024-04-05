#include "ParetoWriter.h"
#include <filesystem>
#include <iostream>
#include <fstream>

void ParetoWriter::WriteParetoToCSV(const char* directoryPath, const std::string& fileName, const ParetoFront& paretoToWrite)
{
	const std::filesystem::path resultsPath = std::filesystem::path(directoryPath) / (fileName + ".csv");
	std::ofstream writeFileStream(resultsPath);

	for (const auto& solution : paretoToWrite.solutions)
	{
		for (size_t i = 0; i < solution.size(); ++i)
		{
			writeFileStream << solution[i];
			if (i < solution.size() - 1)
			{
				writeFileStream << ";";
			}
			else
			{
				writeFileStream << "\n";
			}
		}
	}

	writeFileStream.close();
}

