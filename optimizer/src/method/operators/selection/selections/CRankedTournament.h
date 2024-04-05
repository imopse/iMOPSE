

#pragma once

#include "ASelection.h"
#include "../../../individual/MO/SMOIndividual.h"

class CRankedTournament : public ASelection
{
public:
    explicit CRankedTournament(int tournamentSize) : ASelection(tournamentSize)
    {};
    ~CRankedTournament() override = default;

    SMOIndividual *Select(std::vector<SMOIndividual *> &population);
};
