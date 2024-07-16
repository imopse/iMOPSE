#include <algorithm>
#include "CECVRPTWInitialization.h"
#include "../../../../utils/random/CRandom.h"

SSOIndividual* CECVRPTWInitialization::CreateSOIndividual(SProblemEncoding &encoding)
{
    SGenotype genotype;
    InitGenotype(encoding, genotype);
    float toAdd = encoding.m_Encoding[0].m_SectionDescription[0].m_MinValue;
    float maxCustomerIndex = encoding.m_Encoding[0].m_SectionDescription[0].m_MaxValue;
    for (int i = 0; i < genotype.m_IntGenotype.size(); i++) {
        genotype.m_IntGenotype[i] += toAdd;
    }
    int vehicleCount = m_problem.GetECVRPTWTemplate().GetVehicleCount() - 1;
    for (int i = 1; i <= vehicleCount; i++) {
        auto it = std::find(genotype.m_IntGenotype.begin(), genotype.m_IntGenotype.end(), maxCustomerIndex + i);
        *it = VEHICLE_DELIMITER;
    }
    std::vector<float> emptyEvaluation(encoding.m_objectivesNumber, 0);
    std::vector<float> emptyNormalizedEvaluation(encoding.m_objectivesNumber, 0);

    return new SSOIndividual(genotype, emptyEvaluation, emptyNormalizedEvaluation);
}

SSOIndividual * CECVRPTWInitialization::CreateSOIndividual(SProblemEncoding &encoding, SGenotype &genotype)
{
    std::vector<float> emptyEvaluation(encoding.m_objectivesNumber, 0);
    std::vector<float> emptyNormalizedEvaluation(encoding.m_objectivesNumber, 0);

    return new SSOIndividual(genotype, emptyEvaluation, emptyNormalizedEvaluation);
}

SMOIndividual* CECVRPTWInitialization::CreateMOIndividual(SProblemEncoding &encoding)
{
    SGenotype genotype;
    InitGenotype(encoding, genotype);
    float toAdd = encoding.m_Encoding[0].m_SectionDescription[0].m_MinValue;
    float maxCustomerIndex = encoding.m_Encoding[0].m_SectionDescription[0].m_MaxValue;
    for (int i = 0; i < genotype.m_IntGenotype.size(); i++) {
        genotype.m_IntGenotype[i] += toAdd;
    }
    int vehicleCount = m_problem.GetECVRPTWTemplate().GetVehicleCount() - 1;
    for (int i = 1; i <= vehicleCount; i++) {
        auto it = std::find(genotype.m_IntGenotype.begin(), genotype.m_IntGenotype.end(), maxCustomerIndex + i);
        *it = VEHICLE_DELIMITER;
    }
    std::vector<float> emptyEvaluation(encoding.m_objectivesNumber, 0);
    std::vector<float> emptyNormalizedEvaluation(encoding.m_objectivesNumber, 0);

    return new SMOIndividual(genotype, emptyEvaluation, emptyNormalizedEvaluation);
}

SParticle* CECVRPTWInitialization::CreateParticle(SProblemEncoding &encoding)
{
    SGenotype genotype;
    InitGenotype(encoding, genotype);
    std::vector<float> emptyEvaluation(encoding.m_objectivesNumber, 0);
    std::vector<float> emptyNormalizedEvaluation(encoding.m_objectivesNumber, 0);
    std::vector<float> velocity;

    return new SParticle(genotype, emptyEvaluation, emptyNormalizedEvaluation, velocity);
}

SSOIndividual* CECVRPTWInitialization::CreateNeighborSolution(SProblemEncoding &encoding, const SSOIndividual &baseSolution)
{
    auto* newSolution = new SSOIndividual(baseSolution);

    // Iterate through each encoding section of the problem
    for (auto &section: encoding.m_Encoding)
    {
        switch (section.m_SectionType)
        {
            case EEncodingType::ASSOCIATION:
            {
                int randomIndex = CRandom::GetInt(0, section.m_SectionDescription.size());
                float minVal = section.m_SectionDescription[randomIndex].m_MinValue;
                float maxVal = section.m_SectionDescription[randomIndex].m_MaxValue;
                newSolution->m_Genotype.m_FloatGenotype[randomIndex] = CRandom::GetFloat(minVal, maxVal);
                break;
            }
            case EEncodingType::PERMUTATION:
            {
                int index1 = CRandom::GetInt(0, section.m_SectionDescription.size());
                int index2 = CRandom::GetInt(0, section.m_SectionDescription.size());

                std::swap(newSolution->m_Genotype.m_IntGenotype[index1], newSolution->m_Genotype.m_IntGenotype[index2]);
                break;
            }
            case EEncodingType::BINARY:
            {
                int randomIndex = CRandom::GetInt(0, section.m_SectionDescription.size());
                newSolution->m_Genotype.m_BoolGenotype[randomIndex] = !newSolution->m_Genotype.m_BoolGenotype[randomIndex];
                break;
            }
        }
    }

    return newSolution;
}

void CECVRPTWInitialization::InitGenotype(SProblemEncoding &encoding, SGenotype &genotype) const
{
    for (const SEncodingSection &encodingSection: encoding.m_Encoding)
    {
        switch (encodingSection.m_SectionType)
        {
            case EEncodingType::ASSOCIATION:
            {
                for (const SEncodingDescriptor &encDesc: encodingSection.m_SectionDescription)
                {
                    genotype.m_FloatGenotype.push_back(CRandom::GetFloat(encDesc.m_MinValue, encDesc.m_MaxValue));
                }
                break;
            }
            case EEncodingType::PERMUTATION:
            {
                size_t sectionStart = genotype.m_IntGenotype.size();
                size_t sectionSize = encodingSection.m_SectionDescription.size();
                for (int i = 0; i < sectionSize; ++i)
                {
                    genotype.m_IntGenotype.push_back(i);
                }
                CRandom::Shuffle(int(sectionStart), int(sectionStart + sectionSize), genotype.m_IntGenotype);
                break;
            }
            case EEncodingType::BINARY:
            {
                std::generate_n(std::back_inserter(genotype.m_BoolGenotype),
                                encodingSection.m_SectionDescription.size(),
                                []()
                                { return CRandom::GetBool(); });
            }
        }
    }
}
