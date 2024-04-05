#include "CTTP1.h"

CTTP1::CTTP1(CTTP2 &ttp2) : CTTP2(ttp2)
{
    m_ProblemEncoding.m_objectivesNumber = 1;

    m_MaxObjectiveValues = {
            m_TTPTemplate.GetMaxTravelTime() + (-m_TTPTemplate.GetMinProfit() * m_TTPTemplate.GetRentingRatio()),
    };

    m_MinObjectiveValues = {
            m_TTPTemplate.GetMinTravelTime() + (-m_TTPTemplate.GetMaxProfit() * m_TTPTemplate.GetRentingRatio()),
    };
}

void CTTP1::Evaluate(AIndividual& individual)
{
    CTTP2::Evaluate(individual);
    individual.m_NormalizedEvaluation = {individual.m_Evaluation[1] + (individual.m_Evaluation[0] * m_TTPTemplate.GetRentingRatio())};
}
