#include <iostream> // Standard I/O library
#include <random> // Library for random number generation
#include "SProgramParams.h" // Custom header file for program parameters structure
#include "CProgram.h" // Custom header file for the main program class
#include "utils/logger/CExperimentLogger.h" // Custom header for an experiment logger utility class
#include "utils/fileReader/CReadUtils.h"  // Custom header for a file reader utility class

static const int MIN_REQUIRED_ARGS = 4; // Minimum number of arguments required for the program
static const int OPTIMIZER_CONFIG_INDEX = 1; // Index in argv for optimizer configuration file
static const int PROBLEM_NAME_INDEX = 2; // Index in argv for problem name
static const int PROBLEM_DEFINITION_INDEX = 3; // Index in argv for problem definition file
static const int OUTPUT_DIR_PATH_INDEX = 4; // Index in argv for output path directory where results and experiments will be logged
static const int EXECUTION_COUNT_INDEX = 5; // Index in argv for the number of executions
static const int SEED_INDEX = 6; // Index in argv for the number of executions

static const int DEFAULT_EXECUTIONS_NUMBER = 1; // Default number of executions if not specified

void showErrorAndExit(const char *message, const char *detail = ""); // Function prototype for error handling

int main(int argc, char *argv[])
{
    // Check for minimum number of arguments
    if (argc < MIN_REQUIRED_ARGS)
    {
        // Print usage instructions if not enough arguments
        std::cerr << "Usage: " << argv[0]
                  << " <MethodConfigPath> <ProblemName> <ProblemInstancePath> <OutputDirectory> [ExecutionsCount] [Seed]"
                  << std::endl;
        return -1;
    }

    SProgramParams programParams{}; // Initialize program parameters

    // Check if optimizer configuration file exists
    if (!CReadUtils::fileExists(argv[OPTIMIZER_CONFIG_INDEX]))
    {
        showErrorAndExit("The provided OptimizerConfigPath does not exist: ", argv[OPTIMIZER_CONFIG_INDEX]);
    }
    programParams.m_MethodConfigPath = argv[OPTIMIZER_CONFIG_INDEX]; // Set optimizer configuration path

    programParams.m_ProblemName = argv[PROBLEM_NAME_INDEX]; // Set problem name

    // Check if problem definition file exists
    if (!CReadUtils::fileExists(argv[PROBLEM_DEFINITION_INDEX]))
    {
        showErrorAndExit("The provided ProblemDefinitionPath does not exist: ", argv[PROBLEM_DEFINITION_INDEX]);
    }
    programParams.m_ProblemInstancePath = argv[PROBLEM_DEFINITION_INDEX]; // Set problem definition path

    CExperimentLogger::m_OutputDirPath = argv[OUTPUT_DIR_PATH_INDEX];  // Set output directory

    // Set the number of executions, default if not provided
    programParams.m_ExecutionsCount = (argc > EXECUTION_COUNT_INDEX) ?
                                      std::stoi(argv[EXECUTION_COUNT_INDEX]) :
                                      DEFAULT_EXECUTIONS_NUMBER;

    std::random_device rd;
    programParams.m_Seed = (argc > SEED_INDEX) ? std::stoi(argv[SEED_INDEX]) : rd();
    
    CProgram::Run(programParams);

    return 0;
}

void showErrorAndExit(const char *message, const char *detail)
{
    std::cerr << "Error: " << message << detail << std::endl; // Print error message
    exit(-1); // Exit the program with an error code
}