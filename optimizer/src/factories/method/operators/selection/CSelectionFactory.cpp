#include <regex>
#include "CSelectionFactory.h"
#include "../../../../utils/fileReader/CReadUtils.h"

CFitnessTournament *CSelectionFactory::CreateFitnessTournamentSelection(SConfigMap *configMap)
{
    int tournamentSize = ValidateSelectionAndReturnTournamentSize(configMap, "FitnessTournament");
    return new CFitnessTournament(tournamentSize);
}

CRankedTournament *CSelectionFactory::CreateRankedTournamentSelection(SConfigMap *configMap)
{
    int tournamentSize = ValidateSelectionAndReturnTournamentSize(configMap, "RankedTournament");
    return new CRankedTournament(tournamentSize);
}

CRankedTournament* CSelectionFactory::CreateRankedTournamentSelection(SConfigMap* configMap, std::string& selectionName)
{
    int tournamentSize = ValidateSelectionAndReturnTournamentSize(configMap, selectionName);
    return new CRankedTournament(tournamentSize);
}

CGapSelectionByRandomDim *CSelectionFactory::CreateGapSelection(SConfigMap *configMap, bool bntga)
{
    int tournamentSize = ValidateSelectionAndReturnTournamentSize(configMap, "GapSelection");
    return new CGapSelectionByRandomDim(tournamentSize, bntga);
}

int CSelectionFactory::ValidateSelectionAndReturnTournamentSize(SConfigMap *configMap, std::string selectionName)
{
    std::string rawSelectionString;
    if (!configMap->TakeValue(selectionName, rawSelectionString))
    {
        throw std::runtime_error("Cannot find " + selectionName + " param in configuration");
    }
    auto const parameters = CReadUtils::SplitLine(rawSelectionString);

    if (parameters.empty())
    {
        throw std::runtime_error("Tournament size for " + selectionName + " not provided");
    }

    try
    {
        return std::stoi(parameters[0]);
    }
    catch (const std::invalid_argument &e)
    {
        throw std::runtime_error("Invalid tournament size");
    }
}
