
#include "../../SProblemEncoding.h"
#include "../../../method/individual/SGenotype.h"
#include "../../AProblem.h"
#include "CTSPTemplate.h"

class CTSP : public AProblem {
private:
    CTSPTemplate &m_CTSPTemplate;
    SProblemEncoding m_ProblemEncoding;

public:
    CTSP(CTSPTemplate &tspTemplate) : m_CTSPTemplate(tspTemplate) {
        CreateProblemEncoding();
    }

    SProblemEncoding &GetProblemEncoding() {
        return m_ProblemEncoding;
    }

    void Evaluate(AIndividual& individual) {
        // Calculate the total distance of the tour
        float totalDistance = 0.f;
        size_t citiesSize = m_CTSPTemplate.GetCitiesSize();
        auto &distMtx = m_CTSPTemplate.GetDistMtx();

        for (size_t i = 0; i < citiesSize; ++i) {
            size_t cityIdx = individual.m_Genotype.m_IntGenotype[i];
            size_t nextCityIdx = individual.m_Genotype.m_IntGenotype[(i + 1) % citiesSize];
            totalDistance += distMtx[cityIdx][nextCityIdx];
        }

        individual.m_Evaluation = { totalDistance };
        individual.m_NormalizedEvaluation = { totalDistance / m_CTSPTemplate.m_MaxDistance };
    }

    void LogSolution(AIndividual& individual) override {
        
    }

    void CreateProblemEncoding() {
        size_t citiesSize = m_CTSPTemplate.GetCitiesSize();

        SEncodingSection citiesSection = SEncodingSection {
                std::vector<SEncodingDescriptor>(citiesSize, SEncodingDescriptor{
                        (float) 0, (float) (citiesSize - 1)
                }),
                EEncodingType::PERMUTATION
        };

        m_ProblemEncoding = SProblemEncoding{1, {citiesSection}, m_CTSPTemplate.GetDistMtx()};
    }
};