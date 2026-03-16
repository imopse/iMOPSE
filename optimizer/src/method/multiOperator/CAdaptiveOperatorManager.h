#include "method/multiOperator/AMultiOperator.h"
#include "method/operators/mutation/AMutation.h"

struct SConfigMap;
class AProblem;
class SMOIndividual;

template <typename T> class CCSV;

class CAdaptiveOperatorManager
{
public:
    CAdaptiveOperatorManager(SConfigMap* configMap, AProblem& problem,
                             const std::vector<SMOIndividual*>& population, const std::vector<SMOIndividual*>& archive);
    ~CAdaptiveOperatorManager();
    void Reset();

    void LocalAdaptiveMutation(SMOIndividual* individual, SMOIndividual* parent, SMOIndividual* otherParent);
    void LogOperatorStatsToCSV(int generation, CCSV<float>& csv) const;
    size_t GetOperatorDataCount() const { return m_TotalDataCount; }

private:
    AMultiOperator<AMutation>* m_MultiMutation = nullptr;

    // TODO - for testing
    AMultiOperator<AMutation>* m_SecondaryMultiMutation = nullptr;

    AProblem &m_Problem;

    // References to method key variables
    const std::vector<SMOIndividual*>& m_Population;
    const std::vector<SMOIndividual*>& m_Archive;


    std::vector<std::vector<size_t>> m_OperatorIdToDataIdx;
    size_t m_TotalDataCount = 0;

    // TODO - temp
    std::vector<size_t> m_VariantsAccCredit;
    std::vector<size_t> m_VariantsAccCalls;

    // TODO - keep all operator data (and variants) in individual

    // TODO - for now we narrow down to mutation
    void CreateMutationDataMapping();

    float CalculateCredit(SMOIndividual* individual, SMOIndividual* parent, SMOIndividual* otherParent);

    // TODO - see if it will be used
    void ResetAllArchiveOperatorDataButAccCalls();
    bool IsNonDominatedByArchive(const SMOIndividual* ind) const;
    bool IsNonDominatedByPopulation(const SMOIndividual* ind) const;
    float NonDominatedByPopFrac(const SMOIndividual* ind) const;
    float NonDominatedByArchFrac(const SMOIndividual* ind) const;
    float CalcDominationScoreByPop(const SMOIndividual* ind) const;
    float CalcDominationScoreByArch(const SMOIndividual* ind) const;
    float CalcDominationScore2ByArch(const SMOIndividual* ind) const;

    float CalcFitnessImprovementRateVer1(SMOIndividual* newIndividual, SMOIndividual* oldIndividual);
    float CalcFitnessImprovementRateVer3(SMOIndividual* newIndividual, SMOIndividual* oldIndividual);
    float CalcFitnessImprovementRateVer4(SMOIndividual* newIndividual, SMOIndividual* oldIndividual);
    float CalcFitnessImprovementRateVer5(SMOIndividual* newIndividual, SMOIndividual* oldIndividual);

};