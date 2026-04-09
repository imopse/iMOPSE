#pragma once

#include "method/configMap/SConfigMap.h"
#include "method/methods/SO/AdaptGA/CCompass.h"
#include "method/methods/SO/GA/CGA.h"
#include "method/operators/selection/selections/CFitnessTournament.h"

class AGA : public CGA {
public:
  AGA(std::vector<float> &objectiveWeights, AProblem &evaluator,
      AInitialization &initialization, CFitnessTournament &fitnessTournament,
      ACrossover &crossover, AMutation &mutation, SConfigMap *configMap,
      CompassProvider<AMutation> &mutationSelector,
      CompassProvider<ACrossover> &crossoverSelector);
  void RunOptimization();

private:
  CompassProvider<AMutation> &m_mutationSelector;
  CompassProvider<ACrossover> &m_crossoverSelector;
};
