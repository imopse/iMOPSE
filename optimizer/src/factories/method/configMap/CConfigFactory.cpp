#include "CConfigFactory.h"
#include "../../../utils/fileReader/CReadUtils.h"

SConfigMap *CConfigFactory::CreateConfigMap(const char *path)
{
    auto *configMap = new SConfigMap();

    std::ifstream readFileStream(path);

    std::string line;
    while (std::getline(readFileStream, line))
    {
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());

        // Skip empty lines
        if (line.empty())
        {
            continue;
        }

        // Trim leading whitespace to check for comments
        size_t firstNonSpace = line.find_first_not_of(" \t");
        if (firstNonSpace == std::string::npos)
        {
            // Line contains only whitespace - skip it
            continue;
        }

        // Skip comment lines (starting with # or //)
        if (line[firstNonSpace] == '#' ||
            (firstNonSpace + 1 < line.length() && line[firstNonSpace] == '/' && line[firstNonSpace + 1] == '/'))
        {
            continue;
        }

        std::string keyString, valueString;
        if (CReadUtils::ReadKeyValueString(line, " ", keyString, valueString))
        {
            if (!configMap->AddLine(keyString, valueString))
            {
                std::cerr << "Key: " << keyString << "is already present in the config: " << path << std::endl;
                return nullptr;
            }
        }
        else
        {
            std::cerr << "Cannot parse line: " << line << " while reading config: " << path << std::endl;
            return nullptr;
        }
    }

    readFileStream.close();

    return configMap;
}
