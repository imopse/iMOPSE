#pragma once

#include "../ASOMethod.h"
#include "../../../operators/initialization/AInitialization.h"
#include "../../../operators/crossover/ACrossover.h"
#include "../../../operators/mutation/AMutation.h"
#include "../../../individual/SO/SSOIndividual.h"
#include "../../../configMap/SConfigMap.h"
#include "../../../operators/selection/selections/CFitnessTournament.h"
#include <limits>

class CACO : public ASOMethod {
public:
    explicit CACO(
            AProblem &evaluator,
            AInitialization &initialization,
            std::vector<float> &objectiveWeights
    ): ASOMethod(evaluator, initialization, objectiveWeights) {};

    virtual ~CACO()= default;
};
