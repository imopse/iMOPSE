#include <iostream>
#include <chrono>
#include "CProgram.h"
#include "problem/AProblem.h"
#include "factories/problem/CProblemFactory.h"
#include "utils/random/CRandom.h"
#include "method/AMethod.h"
#include "factories/method/CMethodFactory.h"
#include "utils/logger/CExperimentLogger.h"

// Initialize a static member variable of AMethod to count the number of experiment repetitions for different seeds
int AMethod::m_ExperimentRunCounter = 0;

// The Run method executes the main optimizer logic
void CProgram::Run(const SProgramParams &programParams)
{
    // Create a problem instance using the factory based on provided program parameters
    AProblem *problem = CProblemFactory::CreateProblem(
            programParams.m_ProblemName,
            programParams.m_ProblemInstancePath
    );

    // Create a method instance using the factory based on method configuration path
    AMethod *method = CMethodFactory::CreateMethod(
            programParams.m_MethodConfigPath,
            problem
    );

    // Initialize a random number generator seed
    CRandom::SetSeed(programParams.m_Seed);

    // Loop through the number of repetitions specified in the program parameters
    for (int i = 0; i < programParams.m_RepetitionsCount; i++, AMethod::m_ExperimentRunCounter++)
    {
        CRandom::SetSeed(programParams.m_Seed+i);
        
        CExperimentLogger::CreateOutputDataPrefix();
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::cout << "Optimization run #" << i << " ongoing ..." << std::endl;

        // Run the optimization process and then reset the method for the next iteration
        method->RunOptimization();
        method->Reset();

        // Record the duration time
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "Finished in " << duration.count() << "ms" << std::endl;
    }
    
    // Free the memory allocated for 'method' pointer
    delete method;
}