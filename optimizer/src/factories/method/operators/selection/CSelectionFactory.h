#pragma once

#include "../../../../method/operators/selection/selections/CRankedTournament.h"
#include "../../../../method/operators/selection/selections/CGapSelectionByRandomDim.h"
#include "../../../../method/configMap/SConfigMap.h"
#include "../../../../method/operators/selection/selections/CFitnessTournament.h"

class CSelectionFactory
{
public:
    static CFitnessTournament *CreateFitnessTournamentSelection(SConfigMap *configMap);
    static CRankedTournament *CreateRankedTournamentSelection(SConfigMap *configMap);
    static CGapSelectionByRandomDim *CreateGapSelection(SConfigMap *configMap, bool bntga);
private:
    static int ValidateSelectionAndReturnTournamentSize(SConfigMap *configMap, std::string selectionName);
};