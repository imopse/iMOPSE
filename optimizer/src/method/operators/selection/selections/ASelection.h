#pragma once

class ASelection
{
public:
    explicit ASelection(int tournamentSize) : m_TournamentSize(tournamentSize)
    {};
    virtual ~ASelection() = default;
protected:
    // TODO - we cannot assume a generic selection always uses a tournament size (e.g., roulette)
    int m_TournamentSize;
};
