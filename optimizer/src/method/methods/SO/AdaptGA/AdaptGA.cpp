
#include "method/methods/SO/AdaptGA/AdaptGA.h"
#include "../utils/experiment/CSOExperimentUtils.h"
#include "method/individual/SO/SSOIndividual.h"

AGA::AGA(std::vector<float> &objectiveWeights, AProblem &evaluator,
         AInitialization &initialization, CFitnessTournament &fitnessTournament,
         ACrossover &crossover, AMutation &mutation, SConfigMap *configMap,
         CompassProvider<AMutation> &mutationSelector,
         CompassProvider<ACrossover> &crossoverSelector)
    : CGA(objectiveWeights, evaluator, initialization, fitnessTournament,
          crossover, mutation, configMap),
      m_crossoverSelector(crossoverSelector),
      m_mutationSelector(mutationSelector) {}

void AGA::RunOptimization() {
  int generation = 0;

  for (size_t i = 0; i < m_PopulationSize; ++i) {
    CreateIndividual();
  }

  std::vector<float> solutions_values(this->m_Population.size());
  std::transform(this->m_Population.begin(), this->m_Population.end(),
                 solutions_values.begin(),
                 [](const SSOIndividual *sol) { return sol->m_Fitness; });
  this->m_crossoverSelector.updateWeights(solutions_values);
  this->m_mutationSelector.updateWeights(solutions_values);

  this->m_Mutation = *this->m_mutationSelector.provide();
  this->m_Crossover = *this->m_crossoverSelector.provide();

  CSOExperimentUtils::AddExperimentData(generation, m_Population);

  while (generation < m_GenerationLimit) {
    EvolveToNextGeneration();

    std::vector<float> solutions_values(this->m_Population.size());
    std::transform(this->m_Population.begin(), this->m_Population.end(),
                   solutions_values.begin(),
                   [](const SSOIndividual *sol) { return sol->m_Fitness; });
    this->m_crossoverSelector.updateWeights(solutions_values);
    this->m_mutationSelector.updateWeights(solutions_values);

    this->m_Mutation = *this->m_mutationSelector.provide();
    this->m_Crossover = *this->m_crossoverSelector.provide();

    CSOExperimentUtils::AddExperimentData(generation, m_Population);

    generation++;
  }

  auto *best = CSOExperimentUtils::FindBest(m_Population);
  CSOExperimentUtils::LogResultData(*best, m_Problem);
}
