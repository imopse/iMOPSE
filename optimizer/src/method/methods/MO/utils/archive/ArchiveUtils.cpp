#include "ArchiveUtils.h"
#include "utils/logger/CExperimentLogger.h"
#include "utils/dataStructures/CCSV.h"

#include <ostream>
#include <fstream>
#include <algorithm>
#include <sstream>

#define USE_EVAL_DUPLICATE 1

void ArchiveUtils::CopyToArchiveWithFiltering(const std::vector<SMOIndividual*>& individuals,
                                              std::vector<SMOIndividual*>& archive)
{
    std::vector<const SMOIndividual*> filteredIndividuals;
    filteredIndividuals.reserve(individuals.size());

    // For each new individual, check if not dominated or not dominating
    // We don't have to check if the same solution, equal rewards are discarded
    for (size_t p = 0; p < individuals.size(); ++p)
    {
        const SMOIndividual* newInd = individuals[p];
        bool isDominated = false;
        // Check other new individuals
        size_t i = 0;
        while (!isDominated && i < individuals.size())
        {
            if (p != i)
            {
                isDominated = newInd->IsDominatedBy(individuals[i]);
                if (!isDominated && p < i) // If DuplicateFenotype - leave the last one
                {
                    // Check if duplicated
#if USE_EVAL_DUPLICATE
                    isDominated = newInd->IsDuplicateEvalValue(individuals[i]);
#else
                    isDominated = newInd->IsDuplicateGenotype(individuals[i]);
#endif
                }
            }
            ++i;
        }
        // Check individuals in archive
        i = 0;
        while (!isDominated && i < archive.size())
        {
            //if (p != i)
            {
                isDominated = newInd->IsDominatedBy(archive[i]);
                if (!isDominated)
                {
                    // Chick if duplicated
#if USE_EVAL_DUPLICATE
                    isDominated = newInd->IsDuplicateEvalValue(archive[i]);
#else
                    isDominated = newInd->IsDuplicateGenotype(archive[i]);
#endif
                }
            }

            ++i;
        }
        if (!isDominated)
        {
            filteredIndividuals.push_back(newInd);
        }
    }

    // Now check if already archived individuals are not dominated by new, remove otherwise
    archive.erase(std::remove_if(archive.begin(), archive.end(), [filteredIndividuals](const SMOIndividual* ind)
    {
        for (const SMOIndividual* filteredInd: filteredIndividuals)
        {
            if (ind->IsDominatedBy(filteredInd))
            {
                return true;
            }
        }
        return false;
    }), archive.end());

    // Finally, add new non-dominated individuals to the archive
    archive.reserve(archive.size() + filteredIndividuals.size());
    for (const SMOIndividual* filteredInd: filteredIndividuals)
    {
        // Make deep copy
        archive.push_back(new SMOIndividual(*filteredInd));
    }
}

void ArchiveUtils::CopyToArchiveWithFiltering(const SMOIndividual* individual, std::vector<SMOIndividual*>& archive)
{
    // Check if not dominated
    bool isDominated = false;
    size_t i = 0;
    // Check individuals in archive
    while (!isDominated && i < archive.size())
    {
        isDominated = individual->IsDominatedBy(archive[i]);
        if (!isDominated)
        {
            // Chick if duplicated
#if USE_EVAL_DUPLICATE
            isDominated = individual->IsDuplicateEvalValue(archive[i]);
#else
            isDominated = individual->IsDuplicateGenotype(m_Archive[i]);
#endif
        }
        ++i;
    }

    if (!isDominated)
    {
        // Now check if already archived individuals are not dominated by new, remove otherwise
        archive.erase(std::remove_if(archive.begin(), archive.end(), [individual](const SMOIndividual* ind)
        {
            return ind->IsDominatedBy(individual);
        }), archive.end());

        archive.push_back(new SMOIndividual(*individual));
    }
}

std::vector<std::vector<float>> ArchiveUtils::ToEvaluation(const std::vector<SMOIndividual*>& archive)
{
    std::vector<std::vector<float>> evalValues;
    evalValues.reserve(evalValues.size() + archive.size());
    for (const SMOIndividual* ind: archive)
    {
        evalValues.emplace_back(ind->m_Evaluation);
    }
    return evalValues;
}

void ArchiveUtils::LogParetoFront(const std::vector<SMOIndividual*>& archive)
{
    std::ostringstream oss;
    CCSV<float>::ToCSV(oss, ArchiveUtils::ToEvaluation(archive));
    CExperimentLogger::LogResult(oss.str().c_str());
}