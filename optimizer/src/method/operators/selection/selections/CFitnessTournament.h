#pragma once

#include "ASelection.h"
#include "../../../individual/SO/SSOIndividual.h"

class CFitnessTournament : public ASelection
{
public:
    explicit CFitnessTournament(int tournamentSize)
            : ASelection(tournamentSize)
    {};
    ~CFitnessTournament() override = default;

    SSOIndividual *Select(std::vector<SSOIndividual *> &population);
};
