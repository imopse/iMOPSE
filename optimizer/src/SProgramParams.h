#pragma once

/*
* Program entry params
*/
struct SProgramParams
{
    // Path to the method configuration
    char *m_MethodConfigPath;

    // Problem to solve codename (examples: GA, TS, SA, NSGAII, NTGA2)
    char *m_ProblemName;

    // Path to the problem instance definition
    char *m_ProblemInstancePath;

    // <Optional> Number of executions (runs) per instance
    int m_ExecutionsCount;

    // <Optional> Seed for random generation
    int m_Seed;
};