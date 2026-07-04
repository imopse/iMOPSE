#include "LDEGR.h"
#include "method/methods/SO/utils/experiment/CSOExperimentUtils.h"
#include "utils/logger/ErrorUtils.h"
#include "utils/random/CRandom.h"
#include "method/methods/SO/utils/aggregatedFitness/CAggregatedFitness.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <iostream>
#include <chrono>

CLDEGR::CLDEGR(
        AProblem* evaluator, 
        AInitialization* initialization, 
        SConfigMap* configMap, 
        std::vector<float>* objectiveWeights
) {
    m_Problem = evaluator;
    m_Initialization = initialization;
    m_ObjectiveWeights = objectiveWeights;
    
    configMap->TakeValue("PopulationSizeInit",  m_PopulationSizeInit);
    configMap->TakeValue("PopulationSizeMin",   m_PopulationSizeMin);
    configMap->TakeValue("H",                  m_CrFMemoryListSize);
    configMap->TakeValue("MAXFE",              m_MAXFE);
    configMap->TakeValue("pbestRate",          m_pbestRate);
    configMap->TakeValue("offset",             m_offset);
    ErrorUtils::LowerThanZeroI("LDEGR", "PopulationSizeInit", m_PopulationSizeInit);
    ErrorUtils::LowerThanZeroI("LDEGR", "PopulationSizeMin",  m_PopulationSizeMin);
    ErrorUtils::LowerThanZeroI("LDEGR", "H",                 m_CrFMemoryListSize);
    ErrorUtils::LowerThanZeroI("LDEGR", "MAXFE",             m_MAXFE);
}

void CLDEGR::RunOptimization()
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
    
    auto *best = CSOExperimentUtils::FindBest(m_Population);
    bestFitness = best->m_Fitness;
    LogResultData(*best, *m_Problem);
}

void CLDEGR::CreateInitialPopulation()
{
    SProblemEncoding &enc = m_Problem->GetProblemEncoding();
    m_Population.reserve((size_t)m_PopulationSizeInit);
    for (int i = 0; i < m_PopulationSizeInit; ++i)
    {
        auto *newInd = m_Initialization->CreateSOIndividual(enc);
        EvaluateIndividual(newInd);
        m_Population.push_back(newInd);
    }
}

void CLDEGR::EvolveOneGeneration()
{
    if (m_Population.empty()) return;
    std::vector<SSOIndividual*> newPop(m_Population.size());
    
    std::vector<int> sortedIdx(m_Population.size());
    std::iota(sortedIdx.begin(), sortedIdx.end(), 0);
    std::sort(sortedIdx.begin(), sortedIdx.end(), [&](int a, int b)
    {
        return m_Population[a]->m_Fitness < m_Population[b]->m_Fitness;
    });
    int pBestCount = std::max(1, (int)std::ceil(m_pbestRate * m_Population.size()));

    m_SF.clear(); m_SCR.clear(); m_DeltaFitness.clear();

    for (int i = 0; i < (int)m_Population.size(); ++i)
    {
        if (m_CurrentFE >= m_MAXFE)
        {
            return;
        }
        
        int idx = std::min((int)m_CrFMemoryListSize - 1,
                           (int)CRandom::GetFloat(0.0f, (float)m_CrFMemoryListSize));
        float baseF  = m_MF[idx];
        float baseCR = m_MCR[idx];
        float F_i = std::clamp(CRandom::GetNormalDistribution(baseF, m_offset), 0.0f, 1.0f);
        float CR_i= std::clamp(CRandom::GetNormalDistribution(baseCR, m_offset),0.0f,1.0f);

        int pBestIndex = sortedIdx[std::min(pBestCount-1,
                                            (int)CRandom::GetFloat(0.0f,(float)pBestCount))];
        
        std::vector<SSOIndividual*> pool = m_Population;
        pool.insert(pool.end(), m_Archive.begin(), m_Archive.end());
        int r1, r2;
        do { r1 = (int)CRandom::GetFloat(0.0f, (float)pool.size()); } while(pool[r1]==m_Population[i]||pool[r1]==m_Population[pBestIndex]);
        do { r2 = (int)CRandom::GetFloat(0.0f, (float)pool.size()); } while(pool[r2]==m_Population[i]||pool[r2]==m_Population[pBestIndex]|| r2==r1);

        auto *donor = new SSOIndividual(*m_Population[i]);
        DifferentialEvolutionStep(*donor, *m_Population[pBestIndex], *pool[r1], *pool[r2], F_i, CR_i);
        EvaluateIndividual(donor);

        if (donor->m_Evaluation[0] <= m_Population[i]->m_Evaluation[0]) {
            newPop[i] = donor;
            m_SF.push_back(F_i);
            m_SCR.push_back(CR_i);
            m_DeltaFitness.push_back(m_Population[i]->m_Fitness - donor->m_Fitness);
            if (m_Population[i]->m_Fitness > donor->m_Fitness)
                m_Archive.push_back(new SSOIndividual(*m_Population[i]));
        } else {
            newPop[i] = new SSOIndividual(*m_Population[i]);
            delete donor;
        }
    }

    if (!m_SF.empty()) UpdateSuccessMemory();
    for (auto *ind : m_Population) delete ind;
    m_Population = std::move(newPop);
    UpdatePopulationSize();
    MaintainArchiveSize();
}

void CLDEGR::DifferentialEvolutionStep(
        SSOIndividual &donor,
        const SSOIndividual &pbest,
        const SSOIndividual &r1,
        const SSOIndividual &r2,
        float F,
        float CR
)
{
    int dim = donor.m_Genotype.m_FloatGenotype.size();
    int jrand = CRandom::GetInt(0, dim-1);
    for (int g = 0; g < dim; ++g)
    {
        if (CRandom::GetFloat(0.0f, 1.0f) < CR || g == jrand)
        {
            float val = pbest.m_Genotype.m_FloatGenotype[g] +
                        F * (r1.m_Genotype.m_FloatGenotype[g] - r2.m_Genotype.m_FloatGenotype[g]);
            donor.m_Genotype.m_FloatGenotype[g] = std::clamp(val, 0.0f, 1.0f);
        }
    }
}

void CLDEGR::EvaluateIndividual(SSOIndividual *ind)
{
    m_Problem->Evaluate(*ind);
    CAggregatedFitness::CountFitness(*ind, *m_ObjectiveWeights);
    m_CurrentFE++;
}

void CLDEGR::UpdateSuccessMemory()
{
    float sumW = 0;
    for (float df : m_DeltaFitness) sumW += df;
    std::vector<float> w(m_DeltaFitness.size());
    if (sumW > 0) {
        for (size_t i = 0; i < w.size(); ++i)
            w[i] = m_DeltaFitness[i] / sumW;
    }

    float numF = 0, denF = 0, numCR = 0, denCR = 0;
    for (size_t i = 0; i < m_SF.size(); ++i) {
        numF  += w[i] * m_SF[i] * m_SF[i];
        denF  += w[i] * m_SF[i];
        numCR += w[i] * m_SCR[i] * m_SCR[i];
        denCR += w[i] * m_SCR[i];
    }
    m_MF[m_k]  = (denF  > 0) ? numF  / denF  : 0;
    m_MCR[m_k] = (denCR > 0) ? numCR / denCR : 0;
    m_k = (m_k + 1) % m_CrFMemoryListSize;
    m_DeltaFitness.clear();
}

void CLDEGR::UpdatePopulationSize()
{
    float frac = 1 - ((float) m_CurrentFE / (float) m_MAXFE);
    float targetF = m_PopulationSizeMin + frac * (m_PopulationSizeInit - m_PopulationSizeMin);
    int newSize = int(std::round(targetF));
    if (newSize < (int) m_Population.size())
    {
        std::sort(m_Population.begin(), m_Population.end(), [](auto *a, auto *b)
        {
            return a->m_Fitness < b->m_Fitness;
        });
        for (int i = newSize; i < (int) m_Population.size(); ++i)
        {
            delete m_Population[i];
        }
        m_Population.resize(newSize);
    }
}

void CLDEGR::MaintainArchiveSize()
{
    size_t limit = m_Population.size() * 2;
    if (m_Archive.size() <= limit) return;
    size_t toRemove = m_Archive.size() - limit;
    std::nth_element(m_Archive.begin(), m_Archive.begin() + toRemove, m_Archive.end(), [](auto *a, auto *b){
        return a->m_Fitness < b->m_Fitness;
    });
    for (size_t i = 0; i < toRemove; ++i) delete m_Archive[i];
    m_Archive.erase(m_Archive.begin(), m_Archive.begin() + toRemove);
}
