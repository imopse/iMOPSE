
#include <cstring>
#include "CProblemFactory.h"
#include "MSRCPSP/CMSRCPSP_Factory.h"
#include "TTP/CTTPFactory.h"
#include "CVRP/CCVRPFactory.h"
#include "TSP/CTSPFactory.h"
#include "MSRA/MSRAReader.h"
#include "ECVRPTW/CECVRPTWFactory.h"

AProblem* CProblemFactory::CreateProblem(const char* problemName, const char* problemConfigurationPath)
{
    if (strcmp(problemName, "MSRCPSP_TA") == 0) return CMSRCPSP_Factory::CreateMSRCPSP_TA(problemConfigurationPath, 5);
    if (strcmp(problemName, "MSRCPSP_TA2") == 0) return CMSRCPSP_Factory::CreateMSRCPSP_TA(problemConfigurationPath, 2);
    if (strcmp(problemName, "MSRCPSP_TO") == 0) return CMSRCPSP_Factory::CreateMSRCPSP_TO(problemConfigurationPath, 5);
    if (strcmp(problemName, "MSRCPSP_TO2") == 0) return CMSRCPSP_Factory::CreateMSRCPSP_TO(problemConfigurationPath, 2);
    if (strcmp(problemName, "TSP") == 0) return CTSPFactory::CreateTSP(problemConfigurationPath);
    if (strcmp(problemName, "TTP1") == 0) return CTTPFactory::CreateTTP1(problemConfigurationPath);
    if (strcmp(problemName, "TTP2") == 0) return CTTPFactory::CreateTTP2(problemConfigurationPath);
    if (strcmp(problemName, "CVRP") == 0) return CCVRPFactory::CreateCVRP(problemConfigurationPath);
    if (strcmp(problemName, "MSRA") == 0) return CMSRAReader::CreateMSRA(problemConfigurationPath);
    if (strcmp(problemName, "ECVRPTW") == 0) return CECVRPTWFactory::CreateECVRPTW(problemConfigurationPath);
    
    throw std::runtime_error("Problem name: " + std::string(problemName) + " not supported");
}

void CProblemFactory::DeleteObjects()
{
    CMSRCPSP_Factory::DeleteObjects();
    CTSPFactory::DeleteObjects();
    CTTPFactory::DeleteObjects();
    CCVRPFactory::DeleteObjects();
}
