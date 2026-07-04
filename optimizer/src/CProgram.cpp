#include <iostream>
#include <chrono>
#include "CProgram.h"
#include "problem/AProblem.h"
#include "factories/problem/CProblemFactory.h"
#include "utils/random/CRandom.h"
#include "method/AMethod.h"
#include "factories/method/CMethodFactory.h"
#include "utils/logger/CExperimentLogger.h"

int AMethod::m_ExperimentRunCounter = 0;

void CProgram::Run(const SProgramParams &programParams)
{
    AProblem *problem = CProblemFactory::CreateProblem(
            programParams.m_ProblemName,
            programParams.m_ProblemInstancePath
    );
    
    AMethod *method = CMethodFactory::CreateMethod(
            programParams.m_MethodConfigPath,
            problem
    );
    
    CRandom::SetSeed(programParams.m_Seed);
    
    for (int i = 0; i < programParams.m_RepetitionsCount; i++, AMethod::m_ExperimentRunCounter++)
    {
        CRandom::SetSeed(programParams.m_Seed+i);
        
        CExperimentLogger::CreateOutputDataPrefix();
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::cout << "Optimization run #" << i << " ongoing ..." << std::endl;
        
        method->RunOptimization();
        method->Reset();
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "Finished in " << duration.count() << "ms" << std::endl;
    }
    
    delete method;
}