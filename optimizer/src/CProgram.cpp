#include <iostream>
#include <chrono>
#include "CProgram.h"
#include "problem/AProblem.h"
#include "factories/problem/CProblemFactory.h"
#include "utils/random/CRandom.h"
#include "method/AMethod.h"
#include "factories/method/CMethodFactory.h"
#include "utils/logger/CExperimentLogger.h"

// Initialize a static member variable of AMethod to count the number of experiment runs
int AMethod::m_ExperimentRunCounter = 0;

// Define the 'Run' method of the CProgram class, which executes the main program logic
void CProgram::Run(const SProgramParams &programParams)
{
    // Create a problem instance using the factory pattern based on provided program parameters
    AProblem *problem = CProblemFactory::CreateProblem(
            programParams.m_ProblemName,
            programParams.m_ProblemInstancePath
    );

    // Create a method instance using the factory pattern and the created problem
    AMethod *method = CMethodFactory::CreateMethod(
            programParams.m_MethodConfigPath,
            *problem
    );

    // Initialize a random number generator
    CRandom::SetSeed(programParams.m_Seed);

    // Loop through the number of executions specified in the program parameters
    for (int i = 0; i < programParams.m_ExecutionsCount; i++, AMethod::m_ExperimentRunCounter++)
    {
        CRandom::SetSeed(programParams.m_Seed+i);

        // Create a prefix for output data paths for each experiment run
        CExperimentLogger::CreateOutputDataPrefix();

        // Record the start time of the optimization process
        auto start = std::chrono::high_resolution_clock::now();

        // Output a message indicating the start of an optimization run
        std::cout << "Optimization run #" << i << " ongoing ..." << std::endl;

        // Run the optimization process and then reset the method for the next iteration
        method->RunOptimization();
        method->Reset();

        // Record the end time, calculate, and output the duration of the optimization
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "Finished in " << duration.count() << "ms" << std::endl;
    }

    // Clean up and delete objects created by the problem and method factories
    CProblemFactory::DeleteObjects();
    CMethodFactory::DeleteObjects();

    // Free the memory allocated for 'method' and 'problem' pointers
    delete method;
    delete problem;
}