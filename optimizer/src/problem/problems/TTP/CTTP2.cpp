#include "CTTP2.h"

#define TTP_SAVE_FIXED_GENES 1

CTTP2::CTTP2(CTTPTemplate &ttpBase) : m_TTPTemplate(ttpBase)
{
    CreateProblemEncoding();

    m_MaxObjectiveValues = {
            m_TTPTemplate.GetMaxTravelTime(),
            -m_TTPTemplate.GetMinProfit()
    };

    m_MinObjectiveValues = {
            m_TTPTemplate.GetMinTravelTime(),
            -m_TTPTemplate.GetMaxProfit()
    };
}

SProblemEncoding &CTTP2::GetProblemEncoding()
{
    return m_ProblemEncoding;
}

void CTTP2::Evaluate(AIndividual& individual)
{
    // Build solution
    auto &items = m_TTPTemplate.GetItems();
    auto &cityItems = m_TTPTemplate.GetCityItems();
    auto &distMtx = m_TTPTemplate.GetDistMtx();
    int capacity = m_TTPTemplate.GetCapacity();

    float minSpeed = m_TTPTemplate.GetMinSpeed();
    float maxSpeed = m_TTPTemplate.GetMaxSpeed();

    size_t itemsSize = m_TTPTemplate.GetItemsSize();
    size_t citiesSize = m_TTPTemplate.GetCitiesSize();
    std::vector<bool> selection(itemsSize, false);
    int currWeight = 0;

    // // Left to Right - different fixing heuristic
    // {
    //     size_t i = 0;
    //     while (i < itemsSize)
    //     {
    //         int w = items[i].m_Weight;
    //         if (ttpSolution[citiesSize + i] > 0 && currWeight + w <= capacity)
    //         {
    //             currWeight += w;
    //             selection[i] = true;
    //         }
    //         ++i;
    //     }
    // }

    // Ry ratio
    {
        const auto &itemsRatio = m_TTPTemplate.GetProfitRatioSortedItems();
        size_t i = 0;
        while (i < itemsSize)
        {
            size_t itemIdx = itemsRatio[i];
            int w = items[itemIdx].m_Weight;
            if (individual.m_Genotype.m_BoolGenotype[itemIdx] > 0 && currWeight + w <= capacity)
            {
                currWeight += w;
                selection[itemIdx] = true;
            }
#if TTP_SAVE_FIXED_GENES
            else
            {
                // Save back the zeroed item
                individual.m_Genotype.m_BoolGenotype[itemIdx] = 0;
            }
#endif
            ++i;
        }
    }

    // Evaluate
    float travellingTime = 0.f;
    currWeight = 0;
    int currentProfit = 0;

    // For each city
    for (size_t i = 0; i < citiesSize; ++i)
    {
        size_t cityIdx = individual.m_Genotype.m_IntGenotype[i];
        size_t nextCityIdx = individual.m_Genotype.m_IntGenotype[(i + 1) % citiesSize];

        const std::vector<size_t> &itemsInCity = cityItems[cityIdx];
        for (const size_t &itemIdx: itemsInCity)
        {
            if (selection[itemIdx])
            {
                currWeight += items[itemIdx].m_Weight;
                currentProfit += items[itemIdx].m_Profit;
            }
        }

        float velocity = maxSpeed - ((float) currWeight * ((maxSpeed - minSpeed) / (float) capacity));
        velocity = fmaxf(velocity, minSpeed);
        travellingTime += distMtx[cityIdx][nextCityIdx] / velocity;
    }

    // Assign evaluation values, we assume this bi-objective problem
    individual.m_Evaluation =
            {
                    travellingTime,
                    (float) -currentProfit // invert the profit (min -> optimum)
            };

    // Normalize
    for (int i = 0; i < 2; i++)
    {
        individual.m_NormalizedEvaluation[i] = (individual.m_Evaluation[i] - m_MinObjectiveValues[i]) / (m_MaxObjectiveValues[i] - m_MinObjectiveValues[i]);
    }

}

void CTTP2::LogSolution(AIndividual& individual)
{
    
}

void CTTP2::CreateProblemEncoding()
{
    size_t citiesSize = m_TTPTemplate.GetCitiesSize();
    size_t itemsSize = m_TTPTemplate.GetItemsSize();

    SEncodingSection citiesSection = SEncodingSection
            {
                    // city indices <0, n-1>
                    std::vector<SEncodingDescriptor>(citiesSize, SEncodingDescriptor{
                            (float) 0, (float) (citiesSize - 1)
                    }),
                    EEncodingType::PERMUTATION
            };

    SEncodingSection knapsackSection = SEncodingSection
            {
                    std::vector<SEncodingDescriptor>(itemsSize,
                                                     SEncodingDescriptor{(float) 0, (float) 1}),
                    EEncodingType::BINARY
            };

    m_ProblemEncoding = SProblemEncoding{2, {citiesSection, knapsackSection}, m_TTPTemplate.GetDistMtx()};
}

