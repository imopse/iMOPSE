#include "CDE.h"

#include "../../../../utils/random/CRandom.h"
#include "../utils/experiment/CSOExperimentUtils.h"
#include "../utils/aggregatedFitness/CAggregatedFitness.h"
#include "../../../../utils/logger/ErrorUtils.h"

CDE::CDE(
        std::vector<float>& objectiveWeights,
        AProblem& evaluator,
        AInitialization& initialization,
        SConfigMap* configMap
) : ASOMethod(evaluator, initialization, objectiveWeights)
{
    configMap->TakeValue("PopulationSize", m_PopulationSize);
    m_Population.reserve(m_PopulationSize);
    ErrorUtils::LowerThanZeroI("DE", "PopulationSize", m_PopulationSize);

    configMap->TakeValue("GenerationLimit", m_GenerationLimit);
    ErrorUtils::LowerThanZeroI("DE", "GenerationLimit", m_GenerationLimit);

    configMap->TakeValue("Cr", m_Cr);
    ErrorUtils::OutOfScopeF("DE", "Cr", m_Cr);
        
    configMap->TakeValue("F", m_F);
    ErrorUtils::OutOfScopeF("DE", "F", m_F);
}

void CDE::RunOptimization()
{
    int generation = 0;

    for (size_t i = 0; i < m_PopulationSize; ++i)
    {
        CreateIndividual();
    }

    CSOExperimentUtils::AddExperimentData(generation, m_Population);

    while (generation < m_GenerationLimit)
    {
        EvolveToNextGeneration();
        CSOExperimentUtils::AddExperimentData(generation, m_Population);
        generation++;
    }
    auto* best = CSOExperimentUtils::FindBest(m_Population);
    CSOExperimentUtils::LogResultData(*best, m_Problem);
}

void CDE::CreateIndividual()
{
    SProblemEncoding& problemEncoding = m_Problem.GetProblemEncoding();
    auto* newInd = m_Initialization.CreateSOIndividual(problemEncoding);

    m_Problem.Evaluate(*newInd);
    CAggregatedFitness::CountFitness(*newInd, m_ObjectiveWeights);

    m_Population.push_back(newInd);
}

void CDE::EvolveToNextGeneration()
{
    SGenotype donor;

    for (size_t i = 0; i < m_PopulationSize; i += 2)
    {
        SSOIndividual* donor = m_Population[i];

        size_t r1, r2, r3;

        // Randomly select 3 different individuals, probably there is a better way of doing it
        do
        {
            r1 = CRandom::GetInt(0, m_Population.size());
        } while (r1 == i);
        const auto& gens1 = m_Population[r1]->m_Genotype;

        do
        {
            r2 = CRandom::GetInt(0, m_Population.size());
        } while (r2 == r1 || r2 == i);
        const auto& gens2 = m_Population[r2]->m_Genotype;

        do
        {
            r3 = CRandom::GetInt(0, m_Population.size());
        } while (r3 == r2 || r3 == r1 || r3 == i);
        const auto& gens3 = m_Population[r3]->m_Genotype;

        DifferentialEvolutionStep(donor->m_Genotype, gens1, gens2, gens3);
        SSOIndividual trialIndividual(*m_Population[i]);
        m_Problem.Evaluate(trialIndividual);
        CAggregatedFitness::CountFitness(trialIndividual, m_ObjectiveWeights);

        // One to one selection (if better)
        if (trialIndividual.m_NormalizedEvaluation[0] < m_Population[i]->m_NormalizedEvaluation[0])
        {
            *(m_Population[i]) = trialIndividual;
        }
    }
}

void CDE::DifferentialEvolutionStep(SGenotype& donor, const SGenotype& gens1, const SGenotype& gens2,
                                          const SGenotype& gens3)
{
    size_t sectionStartIndex = 0;
    for (const auto& encodingSection: m_Problem.GetProblemEncoding().m_Encoding)
    {
        const auto& sectionDesc = encodingSection.m_SectionDescription;
        const size_t sectionSize = sectionDesc.size();
        switch (encodingSection.m_SectionType)
        {
            case EEncodingType::ASSOCIATION:
            {
                for (size_t j = 0; j < encodingSection.m_SectionDescription.size(); ++j)
                {
                    if (CRandom::GetFloat(0, 1) < m_Cr)
                    {
                        size_t g = sectionStartIndex + j;
                        donor.m_FloatGenotype[g] = gens1.m_FloatGenotype[g] + m_F * (gens2.m_FloatGenotype[g] - gens3.m_FloatGenotype[g]);
                        // Check constraints, use random if out of bounds
                        if (donor.m_FloatGenotype[g] < sectionDesc[j].m_MinValue || donor.m_FloatGenotype[g] >= sectionDesc[j].m_MaxValue)
                        {
                            donor.m_FloatGenotype[g] = CRandom::GetFloat(sectionDesc[j].m_MaxValue, sectionDesc[j].m_MinValue);
                        }
                    }
                }
                break;
            }
        }
        sectionStartIndex += sectionSize;
    }
}
