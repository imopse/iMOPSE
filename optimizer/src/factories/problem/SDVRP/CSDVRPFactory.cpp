#include "CSDVRPFactory.h"

#include "utils/fileReader/CReadUtils.h"

CSDVRPTemplate *CSDVRPFactory::sdvrpTemplate = nullptr;

CSDVRP *CSDVRPFactory::CreateSDVRP(const char *problemDefinitionPath) {
    sdvrpTemplate = ReadSDVRPTemplate(problemDefinitionPath);
    return new CSDVRP(*sdvrpTemplate);
}

void CSDVRPFactory::DeleteObjects() {
    delete sdvrpTemplate;
}

CSDVRPTemplate *CSDVRPFactory::ReadSDVRPTemplate(const char *problemDefinitionPath) {
    auto *result = new CSDVRPTemplate();

    std::ifstream readFileStream(problemDefinitionPath);
    if (!readFileStream.is_open()) {
        throw std::runtime_error("Cannot open file");
    }

    int dimension = 0;
    int capacity = 0;
    readFileStream >> dimension >> capacity;

    std::vector<int> demands(dimension);
    for (int i = 0; i < dimension; i++) {
        readFileStream >> demands[i];
    }

    std::vector<SCitySDVRP> cities;
    cities.reserve(dimension);

    const SCitySDVRP *depot = nullptr;

    for (int i = 0; i <= dimension; i++) {
        float x, y;
        readFileStream >> x >> y;

        if (i == 0) {
            depot = new SCitySDVRP(i, -1, x, y, 0);
            continue;
        }

        cities.emplace_back(i, i - 1, x, y, demands[i - 1]);
    }

    constexpr int trucks = 1;

    const std::string pathString(problemDefinitionPath);
    const size_t fileNameStartPos = pathString.rfind('/') + 1;
    const size_t fileNameEndPos = pathString.rfind('.');
    result->SetFileName(pathString.substr(fileNameStartPos, fileNameEndPos - fileNameStartPos));
    result->SetData(cities, capacity, trucks, *depot);

    readFileStream.close();
    return result;
}
