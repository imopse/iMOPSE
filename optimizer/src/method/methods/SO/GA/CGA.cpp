
#include "CGA.h"
#include "../utils/aggregatedFitness/CAggregatedFitness.h"
#include "../utils/experiment/CSOExperimentUtils.h"
#include "../../../../utils/logger/ErrorUtils.h"

CGA::CGA(
        std::vector<float> &objectiveWeights,
        AProblem &evaluator,
        AInitialization &initialization,
        CFitnessTournament &fitnessTournament,
        ACrossover &crossover,
        AMutation &mutation,
        SConfigMap *configMap
) : ASOGeneticMethod(evaluator, initialization, crossover, mutation, objectiveWeights),
    m_FitnessTournament(fitnessTournament)
{
    configMap->TakeValue("PopulationSize", m_PopulationSize);
    m_Population.reserve(m_PopulationSize);
    ErrorUtils::LowerThanZeroI("GA", "PopulationSize", m_PopulationSize);

    configMap->TakeValue("GenerationLimit", m_GenerationLimit);
    ErrorUtils::LowerThanZeroI("GA", "GenerationLimit", m_GenerationLimit);
}

void CGA::RunOptimization()
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

void CGA::CreateIndividual()
{
    SProblemEncoding& problemEncoding = m_Problem.GetProblemEncoding();
    auto* newInd = m_Initialization.CreateSOIndividual(problemEncoding);

    m_Problem.Evaluate(*newInd);
    CAggregatedFitness::CountFitness(*newInd, m_ObjectiveWeights);

    m_Population.push_back(newInd);
}

void CGA::EvolveToNextGeneration()
{
    std::vector<SSOIndividual *> children;
    children.reserve(m_PopulationSize);

    for (size_t i = 0; i < m_PopulationSize; i += 2)
    {
        auto* firstParent = m_FitnessTournament.Select(m_Population);
        auto* secondParent = m_FitnessTournament.Select(m_Population);

        auto* firstChild = new SSOIndividual{*firstParent};
        auto* secondChild = new SSOIndividual{*secondParent};

        m_Crossover.Crossover(
                m_Problem.GetProblemEncoding(),
                *firstParent,
                *secondParent,
                *firstChild,
                *secondChild
        );

        m_Mutation.Mutate(m_Problem.GetProblemEncoding(), *firstChild);
        m_Mutation.Mutate(m_Problem.GetProblemEncoding(), *secondChild);

        m_Problem.Evaluate(*firstChild);
        CAggregatedFitness::CountFitness(*firstChild, m_ObjectiveWeights);
        m_Problem.Evaluate(*secondChild);
        CAggregatedFitness::CountFitness(*secondChild, m_ObjectiveWeights);

        children.emplace_back(firstChild);
        children.emplace_back(secondChild);
    }

    for (auto& i: m_Population)
    {
        delete i;
    }

    m_Population.clear();
    m_Population.swap(children);
}