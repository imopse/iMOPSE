#pragma once

#include "../../../configMap/SConfigMap.h"
#include "../ASOMethod.h"

class CALNS : public ASOMethod
{
public:
    CALNS(
        std::vector<float>& objectiveWeights,
        AProblem& evaluator,
        AInitialization& initialization,
        SConfigMap* configMap,
        bool logProgress,
        std::vector<AMutation*>& alnsRemovalMutations,
        std::vector<AMutation*>& alnsInsertionMutations
    );
    ~CALNS() override = default;

    void RunOptimization() override;
    SMOIndividual* RunOptimization(SMOIndividual& individual);

    void UpdateProbabilityTables(std::map<AMutation*, std::tuple<float, int>>& removalOperatorsScores,
        std::vector<float>& removalOperatorsProbabilityDistribution,
        std::map<AMutation*, std::tuple<float, int>>& insertOperatorsScores,
        std::vector<float>& insertionOperatorsProbabilityDistribution
    );

    void UpdateScores(SSOIndividual* current,
        SSOIndividual* best,
        AMutation*& removalOperator,
        AMutation*& insertOperator,
        std::map<AMutation*, std::tuple<float, int>>& removalOperatorsScores,
        std::map<AMutation*, std::tuple<float, int>>& insertOperatorsScores
    );

    bool AcceptWorseSolution(SSOIndividual& generated, SSOIndividual& current, float temperature);

    SSOIndividual* RunALNS(SSOIndividual& parent);

private:
    std::vector<AMutation*>& m_alnsRemovalMutations;
    std::vector<AMutation*>& m_alnsInsertionMutations;

    int m_ALNSIterations = 0;
    int m_ALNSNoImprovementIterations = 0;
    float m_ALNSStartTemperature = 0;
    float m_ALNSTemperatureAnnealingRate = 0;
    int m_ALNSProbabilityStepsIterations = 0;

    bool m_logProgress = 0;
};
