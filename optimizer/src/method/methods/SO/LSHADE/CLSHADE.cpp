#include "CLSHADE.h"
#include "utils/logger/ErrorUtils.h"
#include "utils/random/CRandom.h"
#include "method/methods/SO/utils/aggregatedFitness/CAggregatedFitness.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <iostream>

CLSHADE::CLSHADE(
        AProblem* evaluator, 
        AInitialization* initialization, 
        SConfigMap* configMap, 
        std::vector<float>* objectiveWeights
)
{
    m_Problem = evaluator;
    m_Initialization = initialization;
    m_ObjectiveWeights = objectiveWeights;
    
    configMap->TakeValue("PopulationSizeInit", m_PopulationSizeInit);
    configMap->TakeValue("PopulationSizeMin",  m_PopulationSizeMin);
    configMap->TakeValue("H", m_CrFMemoryListSize);
    configMap->TakeValue("MAXFE", m_MAXFE);
    configMap->TakeValue("pbestRate", m_pbestRate);
    configMap->TakeValue("offset", m_offset);
    ErrorUtils::LowerThanZeroI("LSHADE", "PopulationSizeInit", m_PopulationSizeInit);
    ErrorUtils::LowerThanZeroI("LSHADE", "PopulationSizeMin",  m_PopulationSizeMin);
    ErrorUtils::LowerThanZeroI("LSHADE", "H", m_CrFMemoryListSize);
    ErrorUtils::LowerThanZeroI("LSHADE", "MAXFES", m_MAXFE);
}

void CLSHADE::RunOptimization()
{
    Reset();
    CreateInitialPopulation();
    int gen = 1;
    
    while (m_CurrentFE < m_MAXFE)
    {
        EvolveOneGeneration();
        CSOExperimentUtils::AddExperimentData(gen, m_Population);
        gen++;
    }
    
    auto* best = CSOExperimentUtils::FindBest(m_Population);
    m_Problem->Evaluate(*best);
    bestFitness = best->m_Fitness;
    LogResultData(*best, *m_Problem);
}

void CLSHADE::CreateInitialPopulation()
{
    SProblemEncoding& problemEncoding = m_Problem->GetProblemEncoding();
    m_Population.reserve((size_t)m_PopulationSizeInit);
    for (int i = 0; i < m_PopulationSizeInit; i++)
    {
        auto* newInd = m_Initialization->CreateSOIndividual(problemEncoding);
        EvaluateIndividual(*newInd);
        m_Population.push_back(newInd);
    }
}

void CLSHADE::EvolveOneGeneration()
{
    if (m_Population.empty()) return;
    std::vector<SSOIndividual*> newPop(m_Population.size());
    
    std::vector<int> sortedIdx(m_Population.size());
    std::iota(sortedIdx.begin(), sortedIdx.end(), 0);
    std::sort(sortedIdx.begin(), sortedIdx.end(), [&](int a, int b){
        return m_Population[a]->m_Fitness < m_Population[b]->m_Fitness;
    });


    int pBestCount = std::max<int>(1, std::ceil(m_pbestRate * (float)m_Population.size()));

    m_SF.clear();
    m_SCR.clear();
    m_DeltaFitness.clear();
    for (int i = 0; i < (int)m_Population.size(); i++)
    {
        if (m_CurrentFE >= m_MAXFE)
        {
            return;
        }

        int randomIndex = (int)CRandom::GetFloat(0.0f, (float)m_CrFMemoryListSize);
        if (randomIndex >= (int)m_CrFMemoryListSize) randomIndex = (int)m_CrFMemoryListSize - 1;
        float baseF  = m_MF[randomIndex];
        float baseCR = m_MCR[randomIndex];
        
        float F_i  = CRandom::GetNormalDistribution(baseF, m_offset);
        float CR_i = CRandom::GetNormalDistribution(baseCR, m_offset);
        if (F_i < 0)  F_i = 0; if (F_i > 1.0)  F_i = 1.0;
        if (CR_i < 0) CR_i = 0; if (CR_i > 1.0) CR_i = 1.0;
        
        int pBestIndex = sortedIdx[(int)CRandom::GetFloat(0.0f, (float)pBestCount)];
        if (pBestIndex >= (int)m_Population.size()) pBestIndex = (int)m_Population.size() - 1;
        
        std::vector<SSOIndividual*> unionPool;
        unionPool.reserve(m_Population.size() + m_Archive.size());
        unionPool.insert(unionPool.end(), m_Population.begin(), m_Population.end());
        unionPool.insert(unionPool.end(), m_Archive.begin(),   m_Archive.end());
        
        int r1, r2;
        for(;;)
        {
            r1 = (int)CRandom::GetFloat(0.0f, (float)unionPool.size());
            if (r1 >= (int)unionPool.size()) r1 = (int)unionPool.size() - 1;
            if (unionPool[r1] != m_Population[i] && unionPool[r1] != m_Population[pBestIndex]) break;
        }
        for(;;)
        {
            r2 = (int)CRandom::GetFloat(0.0f, (float)unionPool.size());
            if (r2 >= (int)unionPool.size()) r2 = (int)unionPool.size() - 1;
            if (unionPool[r2] != m_Population[i] &&
                unionPool[r2] != m_Population[pBestIndex] &&
                unionPool[r2] != unionPool[r1]) break;
        }

        auto* donor = new SSOIndividual(*m_Population[i]);

        DifferentialEvolutionStep(
                *donor,
                *m_Population[i],
                *m_Population[pBestIndex],
                *unionPool[r1],
                *unionPool[r2],
                F_i,
                CR_i
        );

        EvaluateIndividual(*donor);

        if (donor->m_Fitness <= m_Population[i]->m_Fitness)
        {
            newPop[i] = donor;
            m_SF.push_back(F_i);
            m_SCR.push_back(CR_i);

            float fitnessImprovement = m_Population[i]->m_Fitness - donor->m_Fitness;
            m_DeltaFitness.push_back(fitnessImprovement);
            
            if (m_Population[i]->m_Fitness > donor->m_Fitness)
            {
                auto* archiveIndv = new SSOIndividual(*m_Population[i]);
                m_Archive.push_back(archiveIndv);
            }
        }
        else
        {
            newPop[i] = new SSOIndividual(*m_Population[i]);
            delete donor;
        }
    }

    if (!m_SF.empty()) UpdateSuccessMemory();
    for (auto* indv: m_Population) {
        delete indv;
    }
    m_Population = newPop;
    UpdatePopulationSize();
    MaintainArchiveSize();
}

void CLSHADE::DifferentialEvolutionStep(
        SSOIndividual& donor,
        const SSOIndividual& target,
        const SSOIndividual& pbest,
        const SSOIndividual& r1,
        const SSOIndividual& r2,
        float F,
        float CR
)
{
    SProblemEncoding& encoding = m_Problem->GetProblemEncoding();
    int dim = (int)donor.m_Genotype.m_FloatGenotype.size();
    int jrand = (int)CRandom::GetFloat(0.0f, (float)dim);
    if (jrand >= dim) jrand = dim - 1;

    for (int g = 0; g < dim; ++g)
    {
        if (CRandom::GetFloat(0.0f, 1.0f) < (float) CR || (int) g == jrand)
        {
            float val = target.m_Genotype.m_FloatGenotype[g] +
                        F * (pbest.m_Genotype.m_FloatGenotype[g] - target.m_Genotype.m_FloatGenotype[g]) +
                        F * (r1.m_Genotype.m_FloatGenotype[g] - r2.m_Genotype.m_FloatGenotype[g]);

            donor.m_Genotype.m_FloatGenotype[g] = std::clamp(val, 0.0f, 1.0f);
        }
    }
}

void CLSHADE::EvaluateIndividual(SSOIndividual& ind)
{
    m_Problem->Evaluate(ind);
    CAggregatedFitness::CountFitness(ind, *m_ObjectiveWeights);
    m_CurrentFE++;
}

void CLSHADE::UpdateSuccessMemory()
{
    float sumWeights = 0.0;
    std::vector<float> weights(m_DeltaFitness.size());
    
    for (float delta : m_DeltaFitness) {
        sumWeights += delta;
    }
    
    if (sumWeights > 0) {
        for (size_t i = 0; i < m_DeltaFitness.size(); ++i) {
            weights[i] = m_DeltaFitness[i] / sumWeights;
        }
    }

    float numeratorF = 0.0, denominatorF = 0.0;
    float numeratorCR = 0.0, denominatorCR = 0.0;
    
    for (size_t i = 0; i < m_SF.size(); ++i) {
        numeratorF += weights[i] * (m_SF[i] * m_SF[i]);
        denominatorF += weights[i] * m_SF[i];

        numeratorCR += weights[i] * (m_SCR[i] * m_SCR[i]);
        denominatorCR += weights[i] * m_SCR[i];
    }
    
    float meanF = (denominatorF > 0) ? (numeratorF / denominatorF) : 0.0f;
    float meanCR = (denominatorCR > 0) ? (numeratorCR / denominatorCR) : 0.0f;

    m_MF[m_k] = meanF;
    m_MCR[m_k] = meanCR;
    
    
    m_DeltaFitness.clear();

    m_k = (m_k + 1) % m_CrFMemoryListSize;
}

void CLSHADE::UpdatePopulationSize()
{
    float frac =  1 - ( (float)m_CurrentFE / (float)m_MAXFE );
    float targetF = m_PopulationSizeMin + frac * (m_PopulationSizeInit - m_PopulationSizeMin);
    int newSize = int(std::round(targetF));
    if (newSize < (int)m_Population.size()) {
        std::sort(m_Population.begin(), m_Population.end(), [](auto *a, auto *b){
            return a->m_Fitness < b->m_Fitness;
        });
        for (int i = newSize; i < (int)m_Population.size(); ++i)
            delete m_Population[i];
        m_Population.resize(newSize);
    }
}


void CLSHADE::MaintainArchiveSize()
{
    if (m_Archive.size() <= m_Population.size() * 2) return;

    int over = m_Archive.size() - m_Population.size() * 2;
    
    std::vector<int> indices(m_Archive.size());
    std::iota(indices.begin(), indices.end(), 0);
    
    CRandom::Shuffle(0, indices.size(), indices);
    
    for (int i = 0; i < over; ++i)
    {
        int idx = indices[i];
        delete m_Archive[idx];
        m_Archive[idx] = nullptr;
    }
    
    m_Archive.erase(std::remove(m_Archive.begin(), m_Archive.end(), nullptr), m_Archive.end());
}

