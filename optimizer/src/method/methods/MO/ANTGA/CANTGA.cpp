#include <algorithm>
#include <sstream>
#include "CANTGA.h"
#include "method/methods/MO/utils/archive/ArchiveUtils.h"
#include "utils/logger/ErrorUtils.h"
#include "factories/method/operators/mutation/CMultiMutationFactory.h"
#include "factories/method/CMethodFactory.h"
#include "utils/dataStructures/CCSV.h"
#include "utils/logger/CExperimentLogger.h"

CANTGA::CANTGA(AProblem& evaluator, AInitialization& initialization,
               ACrossover& crossover, AMutation& mutation, CGapSelectionByRandomDim& gapSelection,
               SConfigMap* configMap)
    : AMOGeneticMethod(evaluator, initialization, crossover, mutation), m_GapSelection(gapSelection)
{
    configMap->TakeValue("PopulationSize", m_PopulationSize);
    ErrorUtils::LowerThanZeroI("ANTGA", "PopulationSize", m_PopulationSize);
    m_Population.reserve(m_PopulationSize);
    m_NextPopulation.reserve(m_PopulationSize);

    configMap->TakeValue("GenerationLimit", m_GenerationLimit);
    ErrorUtils::LowerThanZeroI("ANTGA", "GenerationLimit", m_GenerationLimit);

    m_MultiMutation = CMultiMutationFactory::Create(configMap, evaluator);
}

CANTGA::~CANTGA()
{
    if (m_MultiMutation)
    {
        delete m_MultiMutation;
    }
}

void CANTGA::RunOptimization()
{
    //CCSV<float> m_PopulationHistory(1 + m_Problem.GetProblemEncoding().m_objectivesNumber + 5);
    //CCSV<float> m_ArchiveHistory(1 + m_Problem.GetProblemEncoding().m_objectivesNumber + 5);
    CCSV<float> m_OperatorStats(3 + (m_MultiMutation->GetOperatorCount() * 3));
    CCSV<float> m_HVStats(4);

    m_StartTime = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < m_PopulationSize; ++i)
    {
        SProblemEncoding& problemEncoding = m_Problem.GetProblemEncoding();
        auto* newInd = m_Initialization.CreateMOIndividual(problemEncoding);
        m_Problem.Evaluate(*newInd);
        m_Population.push_back(newInd);
    }

    ArchiveUtils::CopyToArchiveWithFiltering(m_Population, m_Archive);

    while (m_Generation < m_GenerationLimit)
    {
        EvolveToNextGeneration();

        int fet = (m_Generation + 1) * int(m_PopulationSize);

        //LogIndividualsToCSV(m_PopulationHistory, m_NextPopulation);
        //LogIndividualsToCSV(m_ArchiveHistory, m_Archive);
        LogOperatorStatsToCSV(m_OperatorStats);
        LogHVStatsToCSV(m_HVStats);

        for (SMOIndividual* ind: m_Population)
        {
            delete ind;
        }
        m_Population = m_NextPopulation;
        m_NextPopulation.clear();
        m_NextPopulation.reserve(m_Population.size());
        m_Generation++;

        if (fet % 5000 == 0)
        {
            //ArchiveUtils::LogParetoFront(m_Archive, fet);
        }
    }

    ArchiveUtils::LogParetoFront(m_Archive);
    //CExperimentLogger::LogResult(m_PopulationHistory.ToStringStream().str().c_str(), "PopHist.csv");
    //CExperimentLogger::LogResult(m_ArchiveHistory.ToStringStream().str().c_str(), "ArchHist.csv");
    CExperimentLogger::LogResult(m_OperatorStats.ToStringStream().str().c_str(), "OperatorStats.csv");
    CExperimentLogger::LogResult(m_HVStats.ToStringStream().str().c_str(), "HVStats.csv");
}

void CANTGA::Reset()
{
    AMOGeneticMethod::Reset();
    m_Generation = 0;
    m_MultiMutation->ResetAllOperatorData();
}

void CANTGA::EvolveToNextGeneration()
{
    //SetMultiMutationCreditBasedOnArchive();
    //SetMultiMutationRegionsCreditBasedOnArchive();

    //if (m_Generation % 100 == 0)
    {
        //m_MultiMutation->ResetAllOperatorDataButAccCalls();
        //m_MultiMutation->ResetAllOperatorData();
        //m_MultiMutation->MultAllOperatorDataBy(0.5f);
        //m_MultiMutation->MultAllOperatorDataBy(0.99f);
        //ResetAllArchiveOperatorDataButAccCalls();
    }

    const auto& parents = m_GapSelection.Select(
        m_Archive,
        m_Problem.GetProblemEncoding().m_objectivesNumber,
        m_PopulationSize
    );
    for (auto& parentPair : parents)
    {
        CrossoverAndMutate(parentPair.first, parentPair.second);
    }
    ArchiveUtils::CopyToArchiveWithFiltering(m_NextPopulation, m_Archive);
}

void CANTGA::CrossoverAndMutate(SMOIndividual* firstParent, SMOIndividual* secondParent)
{
    auto* firstChild = new SMOIndividual{*firstParent};
    auto* secondChild = new SMOIndividual{*secondParent};

    // Copy operator data
    firstChild->m_OperatorsData = firstParent->m_OperatorsData;
    secondChild->m_OperatorsData = secondParent->m_OperatorsData;

    m_Crossover.Crossover(
        m_Problem.GetProblemEncoding(),
        *firstParent,
        *secondParent,
        *firstChild,
        *secondChild
    );

    // Local Variant
    {
        LocalAdaptiveMutation(firstChild, firstParent, secondParent);
        LocalAdaptiveMutation(secondChild, secondParent, firstParent);
        // Copy information back to parents
        firstParent->m_OperatorsData = firstChild->m_OperatorsData;
        secondParent->m_OperatorsData = secondChild->m_OperatorsData;
    }

    // Global Variant
    //{
    //    GlobalAdaptiveMutation(firstChild, secondChild, firstParent, secondParent);
    //}

    m_NextPopulation.emplace_back(firstChild);
    m_NextPopulation.emplace_back(secondChild);
}

void CANTGA::LocalAdaptiveMutation(SMOIndividual* individual, SMOIndividual* parent, SMOIndividual* otherParent)
{
    float decayFactor = 0.999f;
    if (individual->m_OperatorsData.empty())
    {
        individual->m_OperatorsData = std::vector<SAtomicOperatorData>(m_MultiMutation->GetOperatorCount());
    }
    for (size_t i = 0; i < m_MultiMutation->GetOperatorCount(); ++i)
    {
        m_MultiMutation->GetOperatorData(i).m_Credits = individual->m_OperatorsData[i].m_Credits * decayFactor;
        m_MultiMutation->GetOperatorData(i).m_Calls = individual->m_OperatorsData[i].m_Calls * decayFactor;
    }

    CAtomicOperator<AMutation>* atomicMutation = m_MultiMutation->SelectOperator();
    AMutation* mutation = atomicMutation->Get();
    mutation->Mutate(m_Problem.GetProblemEncoding(), *individual);
    atomicMutation->GetData().m_Calls += 1;
    atomicMutation->GetData().m_AccCalls += 1;

    m_Problem.Evaluate(*individual);
    atomicMutation->GetData().m_Credits += CalculateCredit(individual, parent, otherParent);

    for (size_t i = 0; i < m_MultiMutation->GetOperatorCount(); ++i)
    {
        individual->m_OperatorsData[i].m_Credits = m_MultiMutation->GetOperatorData(i).m_Credits;
        individual->m_OperatorsData[i].m_AccCalls = m_MultiMutation->GetOperatorData(i).m_AccCalls;
        individual->m_OperatorsData[i].m_Calls = m_MultiMutation->GetOperatorData(i).m_Calls;
    }

    size_t operatorId = atomicMutation->GetId();
    individual->m_MetaInfo = {
        (float)operatorId,
        parent->m_MutationIdx.has_value() ? (float)parent->m_MutationIdx.value() : -1.f,
        otherParent->m_MutationIdx.has_value() ? (float)otherParent->m_MutationIdx.value() : -1.f
    };
    individual->m_MutationIdx = operatorId;
}

void CANTGA::GlobalAdaptiveMutation(SMOIndividual* firstChild, SMOIndividual* secondChild, SMOIndividual* firstParent, SMOIndividual* secondParent)
{
    CAtomicOperator<AMutation>* atomicMutation = m_MultiMutation->SelectOperator();
    AMutation* mutation = atomicMutation->Get();
    mutation->Mutate(m_Problem.GetProblemEncoding(), *firstChild);
    mutation->Mutate(m_Problem.GetProblemEncoding(), *secondChild);
    atomicMutation->GetData().m_Calls += 2;
    atomicMutation->GetData().m_AccCalls += 2;

    size_t operatorId = atomicMutation->GetId();
    secondChild->m_MetaInfo = firstChild->m_MetaInfo = {
        (float)operatorId,
        firstParent->m_MutationIdx.has_value() ? (float)firstParent->m_MutationIdx.value() : -1.f,
        secondParent->m_MutationIdx.has_value() ? (float)secondParent->m_MutationIdx.value() : -1.f
    };

    secondChild->m_MutationIdx = firstChild->m_MutationIdx = operatorId;

    m_Problem.Evaluate(*firstChild);
    m_Problem.Evaluate(*secondChild);

    atomicMutation->GetData().m_Credits += CalculateCredit(firstChild, firstParent, secondParent);
    atomicMutation->GetData().m_Credits += CalculateCredit(secondChild, secondParent, firstParent);
}

float CANTGA::CalculateCredit(SMOIndividual* individual, SMOIndividual* parent, SMOIndividual* otherParent)
{
    float credit = 0.f;

    // Dom Parent
    if (parent->IsDominatedBy(individual) || otherParent->IsDominatedBy(individual))
        // NonDom by Parent
        //if (!individual->IsDominatedBy(parent) && !individual->IsDominatedBy(otherParent))
        // NonDom by Population
        //if (IsNonDominatedByPopulation(individual))
        // NonDom by Archive
        //if (IsNonDominatedByArchive(individual))
    {
        credit += 1;
    }

    // NonDom by Population Fraction
    //if (float nonDomFrac = NonDominatedByPopFrac(individual))
    // NonDom by Archive Fraction
    //if (float nonDomFrac = NonDominatedByArchFrac(individual))
    //{
    //  credit += nonDomFrac;
    //}

    // Domination Score by Population
    //credit += CalcDominationScoreByPop(individual);
    // Domination Score by Archive
    //credit += CalcDominationScoreByArch(individual);

    // Fitness Improvement Rate (v3)
    //credit += std::max(0.f, CalcFitnessImprovementRateVer3(individual, parent));
    //credit += std::max(0.f, CalcFitnessImprovementRateVer3(individual, otherParent)); // can calc toward 1 or 2 parents

    return credit;
}

void CANTGA::LogIndividualsToCSV(CCSV<float>& csv, const std::vector<SMOIndividual*>& individuals) const
{
    for (const auto& ind : individuals)
    {
        std::vector<float> newRow = { (float)m_Generation };
        newRow.insert(newRow.end(), ind->m_Evaluation.begin(), ind->m_Evaluation.end());
        newRow.insert(newRow.end(), ind->m_MetaInfo.begin(), ind->m_MetaInfo.end());
        for (const auto& opData : ind->m_OperatorsData)
        {
            newRow.push_back(opData.m_Credits);
            newRow.push_back(opData.m_Calls);
        }
        csv.AddRow(std::move(newRow));
    }
}

void CANTGA::LogOperatorStatsToCSV(CCSV<float>& csv) const
{
    int fet = (m_Generation + 1) * int(m_PopulationSize);
    for (size_t i = 0; i < m_MultiMutation->GetOperatorCount(); ++i)
    {
        std::vector<float> newRow = { (float)m_Generation, float(fet), (float)i };
        newRow.push_back((float)m_MultiMutation->GetOperatorData(i).m_Credits);
        newRow.push_back((float)m_MultiMutation->GetOperatorData(i).m_Calls);
        newRow.push_back((float)m_MultiMutation->GetOperatorData(i).m_AccCalls);
        csv.AddRow(std::move(newRow));
    }
}

void CANTGA::LogHVStatsToCSV(CCSV<float>& csv)
{
    int fet = (m_Generation + 1) * int(m_PopulationSize);
    // It will sort the archive
    float paretoHV = CalcHV(m_Archive, std::vector<float>{ 1.f, 1.f });
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto durationMS = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - m_StartTime);
    std::vector<float> newRow = { (float)m_Generation, paretoHV, (float)durationMS.count(), float(fet) };
    csv.AddRow(std::move(newRow));
}

void CANTGA::ResetAllArchiveOperatorDataButAccCalls()
{
    for (auto* individual : m_Archive)
    {
        if (!individual->m_OperatorsData.empty())
        {
            for (size_t i = 0; i < m_MultiMutation->GetOperatorCount(); ++i)
            {
                individual->m_OperatorsData[i].m_Credits = 0;
                individual->m_OperatorsData[i].m_Calls = 0;
            }
        }
    }
}

void CANTGA::SetMultiMutationCreditBasedOnArchive()
{
    m_MultiMutation->ResetAllOperatorDataButAccCalls();
    for (const auto* individual : m_Archive)
    {
        if (individual->m_MutationIdx.has_value())
        {
            m_MultiMutation->GetOperatorData(individual->m_MutationIdx.value()).m_Credits += 1;
        }
    }
}

bool CANTGA::IsNonDominatedByArchive(const SMOIndividual* ind) const
{
    for (const SMOIndividual* archInd : m_Archive)
    {
        if (ind->IsDominatedBy(archInd) || ind->IsDuplicateEvalValue(archInd))
        {
            return false;
        }
    }
    return true;
}

bool CANTGA::IsNonDominatedByPopulation(const SMOIndividual* ind) const
{
    for (const SMOIndividual* popInd : m_Population)
    {
        if (ind->IsDominatedBy(popInd) || ind->IsDuplicateEvalValue(popInd))
        {
            return false;
        }
    }
    return true;
}

float CANTGA::NonDominatedByPopFrac(const SMOIndividual* ind) const
{
    float nonDomCount = 0.f;
    for (const SMOIndividual* popInd : m_Population)
    {
        if (!ind->IsDominatedBy(popInd) && !ind->IsDuplicateEvalValue(popInd))
        {
            nonDomCount += 1.f;
        }
    }
    return nonDomCount / (float)m_Population.size();
}

float CANTGA::NonDominatedByArchFrac(const SMOIndividual* ind) const
{
    float nonDomCount = 0.f;
    for (const SMOIndividual* archInd : m_Archive)
    {
        if (!ind->IsDominatedBy(archInd) && !ind->IsDuplicateEvalValue(archInd))
        {
            nonDomCount += 1.f;
        }
    }
    return nonDomCount / (float)m_Archive.size();
}

float CANTGA::CalcDominationScoreByPop(const SMOIndividual* ind) const
{
    float domScore = 0.f;
    for (const SMOIndividual* popInd : m_Population)
    {
        if (popInd->IsDominatedBy(ind))
        {
            domScore += 2.f;
        }
        else if (!ind->IsDominatedBy(popInd) && !ind->IsDuplicateEvalValue(popInd))
        {
            domScore += 1.f;
        }
    }
    return domScore / (float)m_Population.size();
}

float CANTGA::CalcDominationScoreByArch(const SMOIndividual* ind) const
{
    float domScore = 0.f;
    for (const SMOIndividual* archInd : m_Archive)
    {
        if (archInd->IsDominatedBy(ind))
        {
            domScore += 2.f;
        }
        else if (!ind->IsDominatedBy(archInd) && !ind->IsDuplicateEvalValue(archInd))
        {
            domScore += 1.f;
        }
    }
    return domScore / (float)m_Archive.size();
}

float CANTGA::CalcDominationScore2ByArch(const SMOIndividual* ind) const
{
    float domScore = 0.f;
    for (const SMOIndividual* archInd : m_Archive)
    {
        if (archInd->IsDominatedBy(ind))
        {
            return 2.f;
        }
        else if (!ind->IsDominatedBy(archInd) && !ind->IsDuplicateEvalValue(archInd))
        {
            domScore += 1.f;
        }
    }
    return domScore / (float)m_Archive.size();
}

float CANTGA::CalcFitnessImprovementRateVer1(SMOIndividual* newIndividual, SMOIndividual* oldIndividual)
{
    // Simple weighted sum
    float newValue = 0.f;
    float oldValue = 0.f;
    const size_t objCount = m_Problem.GetProblemEncoding().m_objectivesNumber;
    for (size_t i = 0; i < objCount; ++i)
    {
        newValue += newIndividual->m_NormalizedEvaluation[i];
        oldValue += oldIndividual->m_NormalizedEvaluation[i];
    }
    // Minimization
    if (oldValue == 0.f) return 0.f;

    return (oldValue - newValue) / oldValue;
}

float CANTGA::CalcFitnessImprovementRateVer3(SMOIndividual* newIndividual, SMOIndividual* oldIndividual)
{
    // Simple weighted sum
    float newValue = 0.f;
    float oldValue = 0.f;
    const size_t objCount = m_Problem.GetProblemEncoding().m_objectivesNumber;
    for (size_t i = 0; i < objCount; ++i)
    {
        newValue += newIndividual->m_NormalizedEvaluation[i] * (oldIndividual->m_NormalizedEvaluation[i] - 1.0f);
        oldValue += oldIndividual->m_NormalizedEvaluation[i] * (oldIndividual->m_NormalizedEvaluation[i] - 1.0f);
    }
    // Minimization
    if (oldValue == 0.f) return 0.f;
    //if (fabs(oldValue) < FLT_EPSILON) return 0.f;

    return (oldValue - newValue) / oldValue;
}

float CANTGA::CalcFitnessImprovementRateVer4(SMOIndividual* newIndividual, SMOIndividual* oldIndividual)
{
    // Simple weighted sum
    float newValue = 0.f;
    float oldValue = 0.f;
    const size_t objCount = m_Problem.GetProblemEncoding().m_objectivesNumber;
    for (size_t i = 0; i < objCount; ++i)
    {
        newValue += (newIndividual->m_NormalizedEvaluation[i] - 1.f) * (oldIndividual->m_NormalizedEvaluation[i] - 1.0f);
        oldValue += (oldIndividual->m_NormalizedEvaluation[i] - 1.f) * (oldIndividual->m_NormalizedEvaluation[i] - 1.0f);
    }
    // Maximization
    if (oldValue <= 0.f) oldValue = FLT_EPSILON;
    //if (fabs(oldValue) < FLT_EPSILON) return 0.f;

    return (newValue - oldValue) / oldValue;
}

float CANTGA::CalcFitnessImprovementRateVer5(SMOIndividual* newIndividual, SMOIndividual* oldIndividual)
{
    // Simple weighted sum
    float newValue = 0.f;
    float oldValue = 0.f;
    const size_t objCount = m_Problem.GetProblemEncoding().m_objectivesNumber;

    // calc vec normal
    float s = 0.f;
    for (size_t i = 0; i < objCount; ++i)
    {
        s += std::powf(oldIndividual->m_NormalizedEvaluation[i], 2);
    }
    s = std::sqrtf(s);

    for (size_t i = 0; i < objCount; ++i)
    {
        newValue += newIndividual->m_NormalizedEvaluation[i] * (1.f - oldIndividual->m_NormalizedEvaluation[i]) * s;
        oldValue += oldIndividual->m_NormalizedEvaluation[i] * (1.f - oldIndividual->m_NormalizedEvaluation[i]) * s;
    }
    // Minimization
    if (oldValue == 0.f) return 0.f;
    //if (fabs(oldValue) < FLT_EPSILON) return 0.f;

    return (oldValue - newValue) / oldValue;
}

// TODO - copied from ParetoAnalyzer - remove it or move somewhere else
// Calculate Hyper-volume using values as they are (either absolute or normalized), using the reference point
float CANTGA::CalcHV(std::vector<SMOIndividual*>& individuals, const std::vector<float>& refPoint)
{
    auto sortLambda = [](const SMOIndividual* lhv, const SMOIndividual* rhv) -> bool
    {
        return lhv->m_NormalizedEvaluation[0] < rhv->m_NormalizedEvaluation[0];
    };

    std::sort(individuals.begin(), individuals.end(), sortLambda);

    float hyperVolume = 0.f;
    float prevCost = refPoint[1];
    for (const SMOIndividual* sol : individuals)
    {
        hyperVolume += ((refPoint[0] - sol->m_NormalizedEvaluation[0]) * (prevCost - sol->m_NormalizedEvaluation[1]));
        prevCost = sol->m_NormalizedEvaluation[1];
    }

    return hyperVolume;
}
