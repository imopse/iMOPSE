#include "ParetoReader.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>

void ParetoReader::ReadConfigParetos(const char* directoryPath, ConfigResults& configResults, const char* instanceName)
{
	const std::filesystem::path resultsPath(directoryPath);

	for (const auto& runDirEntry : std::filesystem::directory_iterator(resultsPath))
	{
		auto instancePath = runDirEntry.path() / "results.csv";
		ParetoFront paretoFront;

		std::ifstream readFileStream(instancePath);

		std::string line;
		std::string word;
		while (std::getline(readFileStream, line))
		{
			std::stringstream lineStream(line);

			std::vector<float> pointVec;
			while (std::getline(lineStream, word, ';'))
			{
				pointVec.push_back(std::stof(word));
			}
			if (!pointVec.empty()) paretoFront.solutions.push_back(pointVec);
		}

		readFileStream.close();

		configResults.AddParetoFront(instanceName, paretoFront);
	}
}

void ParetoReader::ReadTaguchiParetos(const char* directoryPath, std::vector<ConfigData>& configsToAnalyze)
{
	const std::filesystem::path resultsPath(directoryPath);

	for (const auto dirEntry : std::filesystem::directory_iterator(resultsPath))
	{
		auto fileName = dirEntry.path().filename().string();
		if (fileName.rfind("archive.csv") != std::string::npos)
		{
			ParetoFront paretoFront;

			size_t splitPos = fileName.find("_config");
			std::string instanceName = fileName.substr(0, splitPos);

			size_t configIdPos = splitPos + 7; // len("_config")
			size_t runIdPos = fileName.find("_run", configIdPos);

			std::string configName = fileName.substr(configIdPos, runIdPos - configIdPos);

			auto foundConfig = std::find_if(configsToAnalyze.begin(), configsToAnalyze.end(), [configName](const ConfigData& conf) -> bool
			{
				return conf.configName == configName;
			});

			ConfigData* activeConfig = nullptr;
			if (foundConfig == configsToAnalyze.end())
			{
				configsToAnalyze.emplace_back(configName, directoryPath);
				activeConfig = &(configsToAnalyze.back());
			}
			else
			{
				activeConfig = &(*foundConfig);
			}

			std::ifstream readFileStream(dirEntry.path());

			std::string line;
			std::string word;
			while (std::getline(readFileStream, line))
			{
				std::stringstream lineStream(line);

				std::vector<float> pointVec;
				while (std::getline(lineStream, word, ';'))
				{
					pointVec.push_back(std::stof(word));
				}
				paretoFront.solutions.push_back(pointVec);
			}

			readFileStream.close();

			activeConfig->configResults.AddParetoFront(instanceName, paretoFront);
			//std::cout << instanceName << ", " << paretoFront.solutions.size() << std::endl;
		}
	}
}

void ParetoReader::ReadNTGA2Paretos(const char* filePath, ConfigResults& configResults)
{
	std::ifstream readFileStream(filePath);
	size_t prefixLen = 4; // .def

	std::string line;
	std::string word;
	std::string instanceName;
	while (std::getline(readFileStream, line))
	{
		size_t splitPos = line.rfind("/");
		// New instance
		if (splitPos != std::string::npos)
		{
			instanceName = line.substr(splitPos + 1, line.length() - (splitPos + 1) - prefixLen);
		}

		ParetoFront paretoFront;

		while (std::getline(readFileStream, line))
		{
			if (line.empty())
			{
				// end of run
				break;
			}

			std::stringstream lineStream(line);
			std::vector<float> pointVec;
			while (std::getline(lineStream, word, ';'))
			{
				pointVec.push_back(std::stof(word));
			}
			paretoFront.solutions.push_back(pointVec);
		}

		if (paretoFront.solutions.size() > 0)
		{
			//std::cout << instanceName << ", " << paretoFront.solutions.size() << std::endl;
			configResults.AddParetoFront(instanceName, paretoFront);
		}
	}

	readFileStream.close();
}

void ParetoReader::ReadParetoFromCSV(const char* directoryPath, const std::string& fileName, ParetoFront& paretoToRead)
{
	const std::filesystem::path fullParetoPath = std::filesystem::path(directoryPath) / (fileName + ".csv");

	std::ifstream readFileStream(fullParetoPath);

	std::string line;
	std::string word;
	while (std::getline(readFileStream, line))
	{
		std::stringstream lineStream(line);

		std::vector<float> pointVec;
		while (std::getline(lineStream, word, ';'))
		{
			pointVec.push_back(std::stof(word));
		}
		paretoToRead.solutions.push_back(pointVec);
	}

	readFileStream.close();
}