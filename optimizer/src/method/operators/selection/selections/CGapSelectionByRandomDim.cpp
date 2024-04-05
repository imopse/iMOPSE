#include <algorithm>
#include <cfloat>
#include "CGapSelectionByRandomDim.h"
#include "../../../../utils/random/CRandom.h"

std::vector<std::pair<SMOIndividual*, SMOIndividual*>> CGapSelectionByRandomDim::Select(std::vector<SMOIndividual*>& parents, int objectiveNumber, int populationSize)
{
    // Backup in case of 1 solution in archive
    if (parents.size() < 2)
    {
        return { {parents[0], parents[0]} };
    }

    std::vector<std::pair<SMOIndividual*, SMOIndividual*>> selectedParents;
    selectedParents.reserve(populationSize);
    std::vector<float> gapValues = CalculateGapValues(parents, objectiveNumber);

    for (size_t i = 0; i < populationSize; i += 2)
    {
        size_t firstParentIdx = SelectParentIdxByTournament(parents, gapValues);
        size_t secondParentIdx = 0;
        if (m_BNTGA)
        {
            if (firstParentIdx == 0)
            {
                secondParentIdx = 1;
            }
            else if (firstParentIdx == parents.size() - 1)
            {
                secondParentIdx = parents.size() - 2;
            }
            else
            {
                secondParentIdx = firstParentIdx + (CRandom::GetInt(0, 2) == 0 ? 1 : -1);
            }
            parents[firstParentIdx]->OnSelected();
            parents[secondParentIdx]->OnSelected();
        }
        else
        {
            secondParentIdx = firstParentIdx + (CRandom::GetInt(0, 2) == 0 ? 1 : -1);
            if (secondParentIdx < 0 || secondParentIdx >= parents.size())
            {
                secondParentIdx = SelectParentIdxByTournament(parents, gapValues);
            }
        }
        selectedParents.emplace_back(parents[firstParentIdx], parents[secondParentIdx]);
    }
    return selectedParents;
}


std::vector<float> CGapSelectionByRandomDim::CalculateGapValues(std::vector<SMOIndividual*>& parents, int objectiveNumber) const
{
    size_t objectiveId = CRandom::GetInt(0, objectiveNumber);
    std::sort(parents.begin(), parents.end(), [objectiveId](const SMOIndividual* a, const SMOIndividual* b) -> bool
    {
        return a->m_NormalizedEvaluation[objectiveId] < b->m_NormalizedEvaluation[objectiveId];
    });

    // Calculate Gaps
    size_t frontSize = parents.size();
    std::vector<float> gapValues(frontSize, 0.f);

    gapValues[0] = FLT_MAX;
    gapValues[frontSize - 1] = FLT_MAX;

    for (size_t i = 1; i < frontSize - 1; ++i)
    {
        float iValue = parents[i]->m_NormalizedEvaluation[objectiveId];
        gapValues[i] = fmaxf(iValue - parents[i - 1]->m_NormalizedEvaluation[objectiveId],
                             parents[i + 1]->m_NormalizedEvaluation[objectiveId] - iValue);
    }

    if (m_BNTGA)
    {
        for (size_t i = 0; i < frontSize; ++i)
        {
            size_t selected = parents[i]->GetSelected();
            // Gap Balanced
            gapValues[i] = (gapValues[i] / (selected + 1));
            // Gap Balanced norm2 Lambda
            //gapValues[i] = (gapValues[i] / sqrtf(maxGap)) + sqrtf(log((float)globalSelections) / (selected + 1));
            // Gap Balanced norm Lambda
            //gapValues[i] = (gapValues[i] / (selected + 1)) + lambda * sqrtf(log((float)globalSelections * maxSpan) / (selected + 1));
            // Gap Balanced norm2 Lambda
            //gapValues[i] = (gapValues[i] / (selected + 1)) + lambda * sqrtf(log((float)globalSelections) * maxSpan / (selected + 1));
            // Gap Balanced Lambda 1
            //gapValues[i] = (gapValues[i] / (selected + 1)) + lambda * sqrtf(log((float)globalSelections) / (selected + 1));
        }
    }

    return gapValues;
}

size_t CGapSelectionByRandomDim::SelectParentIdxByTournament(const std::vector<SMOIndividual*>& parents, const std::vector<float>& gapValues) const
{
    size_t parentIdx = CRandom::GetInt(0, parents.size());
    float bestGap = gapValues[parentIdx];
    for (size_t i = 1; i < m_TournamentSize; ++i)
    {
        size_t randomIdx = CRandom::GetInt(0, parents.size());
        if (gapValues[randomIdx] > bestGap)
        {
            bestGap = gapValues[randomIdx];
            parentIdx = randomIdx;
        }
    }
    return parentIdx;
}