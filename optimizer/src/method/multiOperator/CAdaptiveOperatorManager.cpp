#include "CAdaptiveOperatorManager.h"
#include "method/individual/MO/SMOIndividual.h"
#include "utils/dataStructures/CCSV.h"
#include "method/multiOperator/operatorSelectors/CUCBMultiOperator.h"
#include "method/multiOperator/operatorSelectors/CUCB2MultiOperator.h"
#include "factories/method/operators/mutation/CMutationFactory.h"

CAdaptiveOperatorManager::CAdaptiveOperatorManager(SConfigMap* configMap, AProblem* problem,
                                                   const std::vector<SMOIndividual*>& population, const std::vector<SMOIndividual*>& archive)
    : m_Problem(problem)
    , m_Population(population)
    , m_Archive(archive)
{
    m_MultiMutation = CMutationFactory::CreateMultiMutation(configMap, problem);
    CreateMutationDataMapping();

    //m_SecondaryMultiMutation = new CCreditRouletteMultiOperator<AMutation>();
    //m_SecondaryMultiMutation = new CUCBMultiOperator<AMutation>();
    m_SecondaryMultiMutation = new CUCB2MultiOperator<AMutation>();
    // Add 3 null operators, they won't do be called, only used for selection mechanism
    m_SecondaryMultiMutation->AddOperator(nullptr);
    m_SecondaryMultiMutation->AddOperator(nullptr);
    m_SecondaryMultiMutation->AddOperator(nullptr);
}

CAdaptiveOperatorManager::~CAdaptiveOperatorManager()
{
    delete m_SecondaryMultiMutation;
    delete m_MultiMutation;
}

void CAdaptiveOperatorManager::Reset()
{
    m_MultiMutation->ResetAllOperatorData();

    m_VariantsAccCredit = std::vector<size_t>(m_TotalDataCount, 0);
    m_VariantsAccCalls = std::vector<size_t>(m_TotalDataCount, 0);
}

void CAdaptiveOperatorManager::LocalAdaptiveMutation(SMOIndividual* individual, SMOIndividual* parent, SMOIndividual* otherParent)
{
    // TODO - make param
    float decayFactor = 0.999f;

    // Setup empty data if missing
    if (individual->m_OperatorsData.empty())
    {
        individual->m_OperatorsData = std::vector<SAtomicOperatorData>(m_TotalDataCount);

//        for (auto& od : individual->m_OperatorsData)
//        {
//            od.m_BaseProb = 1.f / float(m_Problem.GetProblemEncoding().m_Encoding[0].m_SectionDescription.size());
//        }
    }

    for (size_t i = 0; i < m_OperatorIdToDataIdx.size(); ++i)
    {
        float credits = 0.f;
        float calls = 0.f;
        for (size_t dataIdx : m_OperatorIdToDataIdx[i])
        {
            // Apply memory decay and gather counters from all variants
            credits += (individual->m_OperatorsData[dataIdx].m_Credits *= decayFactor);
            calls += (individual->m_OperatorsData[dataIdx].m_Calls *= decayFactor);
        }

        // Setup MultiMutation
        m_MultiMutation->GetOperatorData(i).m_Credits = credits;
        m_MultiMutation->GetOperatorData(i).m_Calls = calls;
    }

    CAtomicOperator<AMutation>* atomicMutation = m_MultiMutation->SelectOperator();
    size_t operatorId = atomicMutation->GetId();
    AMutation* mutation = atomicMutation->Get();
    const auto& operatorDataIds = m_OperatorIdToDataIdx[operatorId];
    size_t variantDataIdx = 0;

    float selProb = 0.f;
    if (operatorDataIds.size() > 1)
    {
        for (size_t i = 0; i < operatorDataIds.size(); ++i)
        {
            const auto& variantData = individual->m_OperatorsData[operatorDataIds[i]];
            m_SecondaryMultiMutation->GetOperatorData(i) = variantData;
        }
        CAtomicOperator<AMutation>* mutationVariant = m_SecondaryMultiMutation->SelectOperator();
        variantDataIdx = operatorDataIds[mutationVariant->GetId()];

        // we assume operator has only one param
        float* paramValue = atomicMutation->Get()->GetParamValue(0);

        switch(mutationVariant->GetId())
        {
            case 0:
                *paramValue = mutationVariant->GetData().m_BaseProb / 2.f;
                break;
            case 1:
                *paramValue = mutationVariant->GetData().m_BaseProb;
                break;
            case 2:
                *paramValue = std::min(mutationVariant->GetData().m_BaseProb * 2.f, 1.f);
                break;
        }
        selProb = *paramValue;
    }
    else
    {
        variantDataIdx = operatorDataIds[0];
    }

    mutation->Mutate(m_Problem->GetProblemEncoding(), *individual);

    // Evaluate and update operator data in the individual
    m_Problem->Evaluate(*individual);
    float creditVal = CalculateCredit(individual, parent, otherParent);
    individual->m_OperatorsData[variantDataIdx].m_Credits += creditVal;
    individual->m_OperatorsData[variantDataIdx].m_Calls += 1;
    individual->m_OperatorsData[variantDataIdx].m_AccCalls += 1;

    m_VariantsAccCredit[variantDataIdx] += creditVal;
    m_VariantsAccCalls[variantDataIdx] += 1;

    individual->m_MetaInfo = {
        (float)variantDataIdx,
        selProb, // TODO - temp
        parent->m_MutationIdx.has_value() ? (float)parent->m_MutationIdx.value() : -1.f,
        otherParent->m_MutationIdx.has_value() ? (float)otherParent->m_MutationIdx.value() : -1.f
    };
    individual->m_MutationIdx = variantDataIdx;

    // TODO - temp
    if (operatorDataIds.size() > 1)
    {
        auto& variantDataLow = individual->m_OperatorsData[operatorDataIds[0]];
        auto& variantDataMid = individual->m_OperatorsData[operatorDataIds[1]];
        auto& variantDataHigh = individual->m_OperatorsData[operatorDataIds[2]];

        float creditScoreLow = ((float)variantDataLow.m_Credits);
        float creditScoreMid = ((float)variantDataMid.m_Credits);
        float creditScoreHigh = ((float)variantDataHigh.m_Credits);

        creditScoreLow += 1;
        creditScoreMid += 1;
        creditScoreHigh += 1;

        // TODO - distribute removed points equally

        float newBaseProb = variantDataMid.m_BaseProb;
        if (creditScoreLow > (creditScoreMid + creditScoreHigh) && creditScoreMid > creditScoreHigh)
        {
            // Move low
            SAtomicOperatorData tempData = variantDataHigh;
            variantDataLow = SAtomicOperatorData();
            variantDataMid = variantDataLow;
            variantDataHigh = variantDataMid;

            newBaseProb /= 2.f;
            variantDataLow.m_BaseProb = newBaseProb;
            variantDataMid.m_BaseProb = newBaseProb;
            variantDataHigh.m_BaseProb = newBaseProb;
        }
        else if (creditScoreHigh > (creditScoreMid + creditScoreLow) && creditScoreMid > creditScoreLow)
        {
            // Move high
            SAtomicOperatorData tempData = variantDataLow;
            variantDataLow = variantDataMid;
            variantDataMid = variantDataHigh;
            variantDataHigh = SAtomicOperatorData();

            newBaseProb *= 2.f;
            variantDataLow.m_BaseProb = newBaseProb;
            variantDataMid.m_BaseProb = newBaseProb;
            variantDataHigh.m_BaseProb = newBaseProb;
        }
    }

}

void CAdaptiveOperatorManager::CreateMutationDataMapping()
{
    size_t dataIdx = 0;
    for (const auto& mutation : m_MultiMutation->GetAtomicOperators())
    {
        switch(mutation.Get()->GetParamCount())
        {
            case 0:
                m_OperatorIdToDataIdx.push_back({ dataIdx++ });
                break;
            case 1:
                m_OperatorIdToDataIdx.push_back({ dataIdx++, dataIdx++, dataIdx++ });
                break;
            default:
                // TODO - add a warning
                break;
        }
    }
    m_TotalDataCount = dataIdx;

    m_VariantsAccCredit = std::vector<size_t>(m_TotalDataCount, 0);
    m_VariantsAccCalls = std::vector<size_t>(m_TotalDataCount, 0);
}

void CAdaptiveOperatorManager::LogOperatorStatsToCSV(int generation, CCSV<float>& csv) const
{
    int fet = (generation + 1) * int(m_Population.size());
//    for (size_t i = 0; i < m_MultiMutation->GetOperatorCount(); ++i)
//    {
//        std::vector<float> newRow = { (float)generation, float(fet), (float)i };
//        newRow.push_back((float)m_MultiMutation->GetOperatorData(i).m_Credits);
//        newRow.push_back((float)m_MultiMutation->GetOperatorData(i).m_Calls);
//        newRow.push_back((float)m_MultiMutation->GetOperatorData(i).m_AccCalls);
//        csv.AddRow(std::move(newRow));
//    }

    for (size_t i = 0; i < m_TotalDataCount; ++i)
    {
        std::vector<float> newRow = { (float)generation, float(fet), (float)i };
        // TODO - update this whole logging, for now we use temp vector
        newRow.push_back((float)m_VariantsAccCredit[i]);
        newRow.push_back((float)m_VariantsAccCalls[i]);
        newRow.push_back((float)m_VariantsAccCalls[i]);
        csv.AddRow(std::move(newRow));
    }
}

float CAdaptiveOperatorManager::CalculateCredit(SMOIndividual* individual, SMOIndividual* parent, SMOIndividual* otherParent)
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

void CAdaptiveOperatorManager::ResetAllArchiveOperatorDataButAccCalls()
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

bool CAdaptiveOperatorManager::IsNonDominatedByArchive(const SMOIndividual* ind) const
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

bool CAdaptiveOperatorManager::IsNonDominatedByPopulation(const SMOIndividual* ind) const
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

float CAdaptiveOperatorManager::NonDominatedByPopFrac(const SMOIndividual* ind) const
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

float CAdaptiveOperatorManager::NonDominatedByArchFrac(const SMOIndividual* ind) const
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

float CAdaptiveOperatorManager::CalcDominationScoreByPop(const SMOIndividual* ind) const
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

float CAdaptiveOperatorManager::CalcDominationScoreByArch(const SMOIndividual* ind) const
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

float CAdaptiveOperatorManager::CalcDominationScore2ByArch(const SMOIndividual* ind) const
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

float CAdaptiveOperatorManager::CalcFitnessImprovementRateVer1(SMOIndividual* newIndividual, SMOIndividual* oldIndividual)
{
    // Simple weighted sum
    float newValue = 0.f;
    float oldValue = 0.f;
    const size_t objCount = m_Problem->GetProblemEncoding().m_objectivesNumber;
    for (size_t i = 0; i < objCount; ++i)
    {
        newValue += newIndividual->m_NormalizedEvaluation[i];
        oldValue += oldIndividual->m_NormalizedEvaluation[i];
    }
    // Minimization
    if (oldValue == 0.f) return 0.f;

    return (oldValue - newValue) / oldValue;
}

float CAdaptiveOperatorManager::CalcFitnessImprovementRateVer3(SMOIndividual* newIndividual, SMOIndividual* oldIndividual)
{
    // Simple weighted sum
    float newValue = 0.f;
    float oldValue = 0.f;
    const size_t objCount = m_Problem->GetProblemEncoding().m_objectivesNumber;
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

float CAdaptiveOperatorManager::CalcFitnessImprovementRateVer4(SMOIndividual* newIndividual, SMOIndividual* oldIndividual)
{
    // Simple weighted sum
    float newValue = 0.f;
    float oldValue = 0.f;
    const size_t objCount = m_Problem->GetProblemEncoding().m_objectivesNumber;
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

float CAdaptiveOperatorManager::CalcFitnessImprovementRateVer5(SMOIndividual* newIndividual, SMOIndividual* oldIndividual)
{
    // Simple weighted sum
    float newValue = 0.f;
    float oldValue = 0.f;
    const size_t objCount = m_Problem->GetProblemEncoding().m_objectivesNumber;

    // calc vec normal
    float s = 0.f;
    for (size_t i = 0; i < objCount; ++i)
    {
        s += powf(oldIndividual->m_NormalizedEvaluation[i], 2);
    }
    s = sqrtf(s);

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
