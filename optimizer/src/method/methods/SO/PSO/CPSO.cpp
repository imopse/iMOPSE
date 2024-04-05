#include <algorithm>
#include "CPSO.h"
#include "../../../../utils/random/CRandom.h"
#include "../utils/experiment/CSOExperimentUtils.h"
#include "../../../../utils/logger/ErrorUtils.h"

CPSO::CPSO(std::vector<float>& objectiveWeights, AProblem& evaluator, AInitialization& initialization, SConfigMap* configMap)
        : ASOMethod(evaluator, initialization, objectiveWeights)
{
    configMap->TakeValue("SwarmSize", m_SwarmSize);
    ErrorUtils::LowerThanZeroI("PSO", "SwarmSize", m_SwarmSize);
    m_Swarm.reserve(m_SwarmSize);

    configMap->TakeValue("InertiaWeight", m_InertiaWeight);
    ErrorUtils::OutOfScopeF("PSO", "InertiaWeight", m_InertiaWeight);

    configMap->TakeValue("CognitiveCoefficient", m_CognitiveCoefficient);
    ErrorUtils::OutOfScopeF("PSO", "CognitiveCoefficient", m_CognitiveCoefficient);

    configMap->TakeValue("SocialCoefficient", m_SocialCoefficient);
    ErrorUtils::OutOfScopeF("PSO", "SocialCoefficient", m_SocialCoefficient);

    configMap->TakeValue("MigrationThreshold", m_MigrationThreshold);
    ErrorUtils::LowerThanZeroF("PSO", "MigrationThreshold", m_MigrationThreshold);

    configMap->TakeValue("IterationLimit", m_IterationLimit);
    ErrorUtils::LowerThanZeroI("PSO", "IterationLimit", m_IterationLimit);
}


void CPSO::RunOptimization()
{
    int iteration = 0;
    
    m_BestKnownFitness = std::numeric_limits<float>::max();

    for (size_t i = 0; i < m_SwarmSize; ++i)
    {
        SParticle* prt = CreateParticle();

        if (prt->m_Fitness < m_BestKnownFitness)
        {
            m_BestKnownFitness = prt->m_Fitness;
            m_BestKnownPosition = prt->m_BestKnownPosition;
        }
    }

    int migrationCounter = 0;

    while (iteration < m_IterationLimit)
    {
        float previousBestKnownFitness = m_BestKnownFitness;
        MoveParticles();
        if (previousBestKnownFitness >= m_BestKnownFitness)
            migrationCounter++;
        if (m_MigrationThreshold > 0 && migrationCounter > m_MigrationThreshold)
        {
            Migrate();
            migrationCounter = 0;
        }

        CSOExperimentUtils::AddExperimentData(iteration, m_Swarm);
        iteration++;
    }

    auto* best = CSOExperimentUtils::FindBest(m_Swarm);
    CSOExperimentUtils::LogResultData(*best, m_Problem);
}

void CPSO::UpdatePosition(std::vector<float>& position, std::vector<float>& velocity)
{
    std::vector<SEncodingDescriptor> sectionDescription = m_Problem.GetProblemEncoding().m_Encoding[0].m_SectionDescription;

    for (size_t i = 0; i < position.size(); ++i)
    {
        float dimension = position[i] + velocity[i];
        dimension = std::max(dimension, sectionDescription[i].m_MinValue);
        dimension = std::min(dimension, 0.9999999f * sectionDescription[i].m_MaxValue);
        position[i] = dimension;
    }
}

void CPSO::UpdateBests(SParticle* particle)
{
    if (particle->m_Fitness < particle->m_BestKnownFitness)
    {
        particle->m_BestKnownPosition = particle->m_Genotype.m_FloatGenotype;
        particle->m_BestKnownFitness = particle->m_Fitness;

        if (particle->m_BestKnownFitness < m_BestKnownFitness)
        {
            m_BestKnownPosition = particle->m_BestKnownPosition;
            m_BestKnownFitness = particle->m_BestKnownFitness;
        }
    }
}

void CPSO::Migrate()
{
    for (size_t i = 0; i < m_SwarmSize; ++i)
    {
        for (size_t j = 0; j < m_Swarm[i]->m_Genotype.m_FloatGenotype.size(); ++j)
        {
            float dimension = m_Problem.GetProblemEncoding().m_Encoding[0].m_SectionDescription[j].m_MaxValue * 0.9999999f - m_Swarm[i]->m_Genotype.m_FloatGenotype[j];
            m_Swarm[i]->m_Genotype.m_FloatGenotype[j] = dimension;
        }

        m_Problem.Evaluate(*m_Swarm[i]);
        CAggregatedFitness::CountFitness(*m_Swarm[i], m_ObjectiveWeights);

        UpdateBests(m_Swarm[i]);
    }
}

SParticle* CPSO::CreateParticle()
{
    SProblemEncoding& problemEncoding = m_Problem.GetProblemEncoding();
    auto* newPart = m_Initialization.CreateParticle(problemEncoding);

    m_Problem.Evaluate(*newPart);
    newPart->m_BestKnownPosition = newPart->m_Genotype.m_FloatGenotype;
    CAggregatedFitness::CountFitness(*newPart, m_ObjectiveWeights);
    newPart->m_BestKnownFitness = newPart->m_Fitness;

    m_Swarm.push_back(newPart);

    return newPart;
}

void CPSO::InitVelocity(SProblemEncoding& problemEncoding, std::vector<float> *newVelocity) const
{
    for (const SEncodingSection& encodingSection : problemEncoding.m_Encoding)
    {
        switch (encodingSection.m_SectionType)
        {
            case EEncodingType::ASSOCIATION:
            {
                for (const SEncodingDescriptor& encDesc : encodingSection.m_SectionDescription)
                {
                    newVelocity->push_back(CRandom::GetFloat(encDesc.m_MinValue, encDesc.m_MaxValue));
                }
                break;
            }
        }
    }
}

void CPSO::MoveParticles()
{
    std::vector<SEncodingDescriptor> sectionDescription = m_Problem.GetProblemEncoding().m_Encoding[0].m_SectionDescription;

    for (size_t i = 0; i < m_SwarmSize; ++i)
    {
        for (size_t j = 0; j < m_Swarm[i]->m_Velocity.size(); ++j)
        {
            float rp = CRandom::GetFloat(0, 1);
            float p = m_Swarm[i]->m_BestKnownPosition[j];
            float rg = CRandom::GetFloat(0, 1);
            float g = m_BestKnownPosition[j];
            float x = m_Swarm[i]->m_Genotype.m_FloatGenotype[j];
            float v = m_Swarm[i]->m_Velocity[j];

            float dimension =
                m_InertiaWeight * m_Swarm[i]->m_Velocity[j]
                + m_CognitiveCoefficient * rp * (p - x)
                + m_SocialCoefficient * rg * (g - x);

            m_Swarm[i]->m_Velocity[j] = std::max(dimension, (sectionDescription[j].m_MinValue - sectionDescription[j].m_MaxValue) * 0.9999999f);
            m_Swarm[i]->m_Velocity[j] = std::min(dimension, (sectionDescription[j].m_MaxValue - sectionDescription[j].m_MinValue) * 0.9999999f);
        }

        UpdatePosition(m_Swarm[i]->m_Genotype.m_FloatGenotype, m_Swarm[i]->m_Velocity);
        m_Problem.Evaluate(*m_Swarm[i]);
        CAggregatedFitness::CountFitness(*m_Swarm[i], m_ObjectiveWeights);

        UpdateBests(m_Swarm[i]);
    }
}