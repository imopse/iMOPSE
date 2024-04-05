#pragma once

#include "../../../individual/MO/SMOIndividual.h"
#include "ASelection.h"

class CGapSelectionByRandomDim : public ASelection
{
public:
    // TODO - if we have dynamic selection object, we can get rid of boolean "bntga" parameter, or make it a set of functions called by the method
    explicit CGapSelectionByRandomDim(int tournamentSize, bool bntga) : ASelection(tournamentSize), m_BNTGA(bntga)
    {};
    ~CGapSelectionByRandomDim() override = default;

    std::vector<std::pair<SMOIndividual*, SMOIndividual*>> Select(std::vector<SMOIndividual*>& parents, int objectiveNumber, int populationSize);
private:
    bool m_BNTGA;

    std::vector<float> CalculateGapValues(std::vector<SMOIndividual*>& parents, int objectiveNumber) const;
    size_t SelectParentIdxByTournament(const std::vector<SMOIndividual*>& parents, const std::vector<float>& gapValues) const;
};
