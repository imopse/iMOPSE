#include "CECVRPTW.h"
#include "../../../utils/logger/CExperimentLogger.h"
#include "CECVRPTWSolution.h"
#include <iostream>
#include <sstream>

CECVRPTW::CECVRPTW(CECVRPTWTemplate& ecvrptwBase)
    : m_ECVRPTWTemplate(ecvrptwBase)
{
    CreateProblemEncoding();

    m_MaxObjectiveValues = {
        m_ECVRPTWTemplate.GetMaxDistance(),
        m_ECVRPTWTemplate.GetMaxDueTime() * (float)m_ECVRPTWTemplate.GetCustomers().size()
    };

    m_MinObjectiveValues = {
            0, //min distance
            0, //min due time
    };
}

std::vector<int> CECVRPTW::GetRealPath(AIndividual& individual)
{
    CECVRPTWSolution solution(m_ECVRPTWTemplate);
    solution.BuildSolution(individual.m_Genotype.m_IntGenotype);
    return solution.GetSolution();
}

void CECVRPTW::Evaluate(AIndividual& individual) 
{
    CECVRPTWSolution solution(m_ECVRPTWTemplate);
    solution.BuildSolution(individual.m_Genotype.m_IntGenotype);

    individual.m_Evaluation[0] = solution.GetTotalDistance();
    individual.m_Evaluation[1] = solution.GetTotalDuration();

    // Normalize
    for (int i = 0; i < 2; i++)
    {
        individual.m_NormalizedEvaluation[i] = (individual.m_Evaluation[i] - m_MinObjectiveValues[i]) / (m_MaxObjectiveValues[i] - m_MinObjectiveValues[i]);
    }
}

void CECVRPTW::CreateProblemEncoding()
{
    auto& customers = m_ECVRPTWTemplate.GetCustomers();

    SEncodingSection citiesSection = SEncodingSection
    {
        std::vector<SEncodingDescriptor>(customers.size() + m_ECVRPTWTemplate.GetVehicleCount() - 1,
            SEncodingDescriptor{
                    (float)customers[0], (float)customers[customers.size()-1]
            }
        ),
        EEncodingType::PERMUTATION
    };

    m_ProblemEncoding = SProblemEncoding{3, {citiesSection} };
}

void CECVRPTW::LogSolution(AIndividual& individual)
{
    auto realPath = GetRealPath(individual);
    std::string solution;
    for (int i = 0; i < realPath.size(); i++) {
        solution += std::to_string(realPath[i]);
        if (i != realPath.size() - 1) {
            solution += ";";
        }
    }
    CExperimentLogger::AddLine(solution.c_str());
}

void CECVRPTW::LogAdditionalData()
{
    std::ostringstream pointsData;
    auto& cityData = m_ECVRPTWTemplate.GetCities();
    for (auto& city : cityData) {
        
        pointsData << city.m_PosX << ';' << city.m_PosY << ';' << (char)city.m_Type << std::endl;
    }
    CExperimentLogger::LogResult(pointsData.str().c_str(), "points.csv");
}

