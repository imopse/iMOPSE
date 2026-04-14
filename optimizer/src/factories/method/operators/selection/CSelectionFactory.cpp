#include <regex>
#include <cstring>
#include "CSelectionFactory.h"
#include "../../../../utils/fileReader/CReadUtils.h"

static const char* const SELECTION = "Selection";

CFitnessTournament *CSelectionFactory::CreateFitnessTournamentSelection(SConfigMap *configMap)
{
    std::string selectionData;
    bool failed = !configMap->TakeValue("FitnessTournament", selectionData);
    if (failed){ return nullptr; }

    auto const vec = CReadUtils::SplitLine(selectionData);
    if (vec.size() != 1) { return nullptr; }

    int tournamentSize = std::stoi(vec[0]);
    return new CFitnessTournament(tournamentSize);
}

CRankedTournament *CSelectionFactory::CreateRankedTournamentSelection(SConfigMap *configMap)
{
    std::string selectionData;
    bool failed = !configMap->TakeValue("RankedTournament", selectionData);
    if (failed){ return nullptr; }

    auto const vec = CReadUtils::SplitLine(selectionData);
    if (vec.size() != 1) { return nullptr; }

    int tournamentSize = std::stoi(vec[0]);
    return new CRankedTournament(tournamentSize);
}

CGapSelectionByRandomDim *CSelectionFactory::CreateGapSelection(SConfigMap *configMap, bool bntga)
{
    std::string selectionData;
    bool failed = !configMap->TakeValue("GapSelection", selectionData);
    if (failed){ return nullptr; }

    auto const vec = CReadUtils::SplitLine(selectionData);
    if (vec.size() != 1) { return nullptr; }

    int tournamentSize = std::stoi(vec[0]);
    return new CGapSelectionByRandomDim(tournamentSize, bntga);
}

ASelection *CSelectionFactory::Create(SConfigMap *configMap, bool bntga)
{
    std::string selectionData;
    if (!configMap->TakeValue(SELECTION, selectionData))
    {
        return nullptr;
    }

    auto const vec = CReadUtils::SplitLine(selectionData);
    if (vec.empty())
    {
        return nullptr;
    }

    const char *opName = vec[0].c_str();

    if (vec[0] == "FitnessTournament")
    {
        if (vec.size() != 2) { return nullptr; }

        int tournamentSize = std::stoi(vec[1]);
        return new CFitnessTournament(tournamentSize);
    }
    else if (vec[0] == "RankedTournament")
    {
        if (vec.size() != 2) { return nullptr; }

        int tournamentSize = std::stoi(vec[1]);
        return new CRankedTournament(tournamentSize);
    }
    else if (vec[0] == "GapSelection")
    {
        if (vec.size() != 2) { return nullptr; }

        int tournamentSize = std::stoi(vec[1]);
        return new CGapSelectionByRandomDim(tournamentSize, bntga);
    }

    return nullptr;
}