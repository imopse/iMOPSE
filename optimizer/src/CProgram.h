#pragma once

#include <string>
#include "SProgramParams.h"

/**
 * @brief Class representing the main program.
 * 
 * CProgram is responsible for executing the main logic of the application.
 * It utilizes the SProgramParams structure to configure and run the program.
 */
class CProgram
{
public:
    /**
     * @brief Executes the main program logic.
     * 
     * This static method takes program parameters and performs the core
     * functionality of the application. It may involve initializing
     * components, setting up environments, running optimization routines,
     * or other tasks as defined in the implementation.
     * 
     * @param programParams A constant reference to SProgramParams struct.
     *                      This struct contains configuration and parameters
     *                      required for program execution.
     */
    static void Run(const SProgramParams &programParams);
};