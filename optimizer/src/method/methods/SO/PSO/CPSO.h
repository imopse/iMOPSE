#pragma once

#include "../../../operators/initialization/AInitialization.h"
#include "../../../configMap/SConfigMap.h"
#include "../../../individual/SO/SParticle.h"
#include "../CAggregatedFitness.h"
#include "../ASOMethod.h"

class CPSO : public ASOMethod
{
public:
    CPSO(
        std::vector<float>& objectiveWeights,
        AProblem& evaluator,
        AInitialization& initialization,
        SConfigMap* configMap
    );
    ~CPSO() override = default;

    void RunOptimization() override;

    void Reset()
    {
        for (auto& i : m_Swarm)
        {
            delete i;
        }
        m_Swarm.clear();
    };
private:
    size_t m_IterationLimit = 0;
    size_t m_SwarmSize = 0;
    float m_InertiaWeight = 0.0f;
    float m_CognitiveCoefficient = 0.0f;
    float m_SocialCoefficient = 0.0f;

    std::vector<SParticle*> m_Swarm;
    std::vector<float> m_BestKnownPosition;
    float m_BestKnownFitness = 0.0f;

    int m_MigrationThreshold = 0;

    SParticle* CreateParticle();
    void MoveParticles();
    void UpdatePosition(std::vector<float>& position, std::vector<float>& velocity);
    void UpdateBests(SParticle* particle);
    void Migrate();
    void InitVelocity(SProblemEncoding& problemEncoding, std::vector<float> *newVelocity) const;
};
