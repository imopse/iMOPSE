
#include <cstring>
#include "CProblemFactory.h"
#include "MSRCPSP/CMSRCPSP_Factory.h"
#include "TTP/CTTPFactory.h"
#include "CVRP/CCVRPFactory.h"
#include "TSP/CTSPFactory.h"

// Define the static method 'CreateProblem' in the 'CProblemFactory' class
// This method creates instances of different problem types based on the provided problem name
AProblem *CProblemFactory::CreateProblem(const char *problemName, const char *problemConfigurationPath)
{
    if (strcmp(problemName, "MSRCPSP_TA") == 0) return CMSRCPSP_Factory::CreateMSRCPSP_TA(problemConfigurationPath, 5);
    if (strcmp(problemName, "MSRCPSP_TA2") == 0) return CMSRCPSP_Factory::CreateMSRCPSP_TA(problemConfigurationPath, 2);
    if (strcmp(problemName, "MSRCPSP_TO") == 0) return CMSRCPSP_Factory::CreateMSRCPSP_TO(problemConfigurationPath, 5);
    if (strcmp(problemName, "MSRCPSP_TO2") == 0) return CMSRCPSP_Factory::CreateMSRCPSP_TO(problemConfigurationPath, 2);
    if (strcmp(problemName, "TSP") == 0) return CTSPFactory::CreateTSP(problemConfigurationPath);
    if (strcmp(problemName, "TTP1") == 0) return CTTPFactory::CreateTTP1(problemConfigurationPath);
    if (strcmp(problemName, "TTP2") == 0) return CTTPFactory::CreateTTP2(problemConfigurationPath);
    if (strcmp(problemName, "CVRP") == 0) return CCVRPFactory::CreateCVRP(problemConfigurationPath);

    // If none of the above conditions are met, throw a runtime error indicating the problem name is not supported
    throw std::runtime_error("Problem name: " + std::string(problemName) + " not supported");
}

// Define the static method 'DeleteObjects' in the 'CProblemFactory' class
// This method is responsible for cleaning up and deleting objects created by the factory
void CProblemFactory::DeleteObjects()
{
    // Call the DeleteObjects method of the CMSRCPSP_TA_Factory to clean up its objects
    CMSRCPSP_Factory::DeleteObjects();

    // Call the DeleteObjects method of the CTSPFactory to clean up its objects
    CTSPFactory::DeleteObjects();
    
    // Call the DeleteObjects method of the CTTPFactory to clean up its objects
    CTTPFactory::DeleteObjects();

    // Call the DeleteObjects method of the CCVRPFactory to clean up its objects
    CCVRPFactory::DeleteObjects();
}
