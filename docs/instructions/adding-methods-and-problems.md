---
layout: default
title: How to add new methods and problems to iMOPSE?
permalink: /docs/instructions/how_to_add_new_code/
parent: Instructions
nav_order: 3
---

# Adding New Methods and Problems

This page describes the usual extension path for adding a new optimization method or a new optimization problem to iMOPSE.

The optimizer is built around two main abstractions:

- `AMethod` — the interface for optimization algorithms.
- `AProblem` — the interface for optimization problems.

At runtime, the optimizer first creates the requested problem, then creates the requested method for that problem. The concrete classes are selected based on configurations.

The optimizer executable is called with the following structure:

```bash
./imopse <MethodConfigPath> <ProblemName> <ProblemDefinitionPath> <OutputDirectory> [ExecutionsCount] [Seed]
```

Internally, the program performs this sequence:

1. `CProblemFactory::CreateProblem(...)` creates an `AProblem`.
2. `CMethodFactory::CreateMethod(...)` reads the method configuration and creates an `AMethod`.
3. The method runs optimization on the problem.
4. The method is reset between repeated runs.
5. Results are written to the selected output directory.

This means every new method or problem must be registered in the correct factory before it can be used from the command line.

---

# Adding a new method

## 1. Create the method directory

Create a directory under either the single-objective or multi-objective method tree.

Use `MO` directory for multi-objective methods and `SO` directory for single-objective methods.

```text
optimizer/src/method/methods/MO/MY_METHOD/
```

or:

```text
optimizer/src/method/methods/SO/MY_METHOD/
```

Example:

```text
optimizer/src/method/methods/MO/MY_METHOD/CMyMethod.h
optimizer/src/method/methods/MO/MY_METHOD/CMyMethod.cpp
```

## 2. Implement `AMethod`

Every method must implement:

```cpp
virtual void RunOptimization() = 0;
virtual void Reset() = 0;
```

Minimal skeleton:

```cpp
#pragma once

#include "method/AMethod.h"
#include "method/configMap/SConfigMap.h"

class CMyMethod : public AMethod
{
public:
    CMyMethod(
        AProblem* problem,
        AInitialization* initialization,
        SConfigMap* configMap
    );

    void RunOptimization() override;
    void Reset() override;

private:
    AProblem* m_Problem;
    AInitialization* m_Initialization;
    SConfigMap* m_ConfigMap;

    int m_GenerationLimit = 0;
    int m_PopulationSize = 0;
};
```

Example implementation outline:

```cpp
#include "CMyMethod.h"

CMyMethod::CMyMethod(
    AProblem* problem,
    AInitialization* initialization,
    SConfigMap* configMap
)
    : m_Problem(problem)
    , m_Initialization(initialization)
    , m_ConfigMap(configMap)
{
    configMap->TakeValue("GenerationLimit", m_GenerationLimit);
    configMap->TakeValue("PopulationSize", m_PopulationSize);
}

void CMyMethod::RunOptimization()
{
    // 1. Create initial solution or population.
    // 2. Evaluate individuals with m_Problem->Evaluate(individual).
    // 3. Apply variance operators.
    // 4. Save final results through the existing logging mechanism.
}

void CMyMethod::Reset()
{
    // Clear method state before the next experiment repetition.
}
```

The exact constructor should follow the style of existing methods. If the method uses crossover, mutation, selection, or objective weights, pass those dependencies through the constructor in the same way as existing methods such as `CGA`, `CNTGA2`, `CNSGAII`, `CMOEAD`, `CBNTGA`, or `CSPEA2`.

## 3. Decide which operators the method needs

Many methods reuse existing operators:

- initialization operators
- crossover operators
- mutation operators
- selection operators
- specialized operators

If the method can reuse existing operators, create them in `CMethodFactory` and pass them to your method constructor.

If the method requires new operators, implement the operator and register it in the relevant operator factory.

## 4. Register the method in `CMethodFactory`

Open:

```text
optimizer/src/factories/method/CMethodFactory.cpp
```

Add an include for the new method:

```cpp
#include "method/methods/MO/MY_METHOD/CMyMethod.h"
```

Then register the method name inside `CMethodFactory::CreateMethod(...)`.

For a method that needs only initialization and a config map:

```cpp
if (methodName == "MY_METHOD")
{
    return new CMyMethod(problem, initialization, configMap);
}
```

For a method that also needs crossover and mutation, register it after the factory creates those operators:

```cpp
if (methodName == "MY_METHOD")
{
    return new CMyMethod(
        problem,
        initialization,
        crossover,
        mutation,
        configMap
    );
}
```

The string used here is the same value that must appear in the method configuration file:

```text
MethodName MY_METHOD
```

## 5. Create a method configuration file

Create a configuration file under:

```text
configurations/methods/MY_METHOD/
```

Example:

```text
configurations/methods/MY_METHOD/MY_METHOD_MSRCPSP.cfg
```

Example:

```text
MethodName MY_METHOD
GenerationLimit 100
PopulationSize 50
Crossover UniformCX 0.6
Mutation RandomBit 0.01
```

## 6. Run the method

Example command:

```bash
cd optimizer/build

./imopse \
  ../../configurations/methods/MY_METHOD/MY_METHOD_MSRCPSP.cfg \
  MSRCPSP_TA2 \
  ../../configurations/problems/MSRCPSP/Regular/200_20_150_9_D5.def \
  ../experiments/MY_METHOD/200_20_150_9_D5/ \
  1 \
  0
```

## Method checklist

Before opening a pull request or committing the method, verify that:

- the method class derives from `AMethod`;
- `RunOptimization()` performs the full optimization process;
- `Reset()` clears all run-specific state;
- all required parameters are documented in the `.cfg` file;
- the method is registered in `CMethodFactory`;
- the `MethodName` in the `.cfg` file exactly matches the factory string;
- the method works with at least one existing problem;
- repeated runs with a fixed seed are reproducible enough for debugging;
- output files can be analyzed by the existing tools.

---

# Adding a new problem

## 1. Create the problem directory

Create a new directory under:

```text
optimizer/src/problem/problems/
```

Example:

```text
optimizer/src/problem/problems/MY_PROBLEM/
```

Suggested files:

```text
optimizer/src/problem/problems/MY_PROBLEM/CMyProblem.h - containing encoding definition and evaluation function
optimizer/src/problem/problems/MY_PROBLEM/CMyProblem.cpp
optimizer/src/problem/problems/MY_PROBLEM/CMyProblemTemplate.h - containing structure of problem and helper methods for evaluation
optimizer/src/problem/problems/MY_PROBLEM/CMyProblemTemplate.cpp
```

## 2. Implement `AProblem`

Every problem must implement:

```cpp
virtual SProblemEncoding& GetProblemEncoding() = 0;
virtual void Evaluate(AIndividual& individual) = 0;
virtual void LogSolution(AIndividual& individual) = 0;
virtual void LogAdditionalData() = 0;
```

Minimal skeleton:

```cpp
#pragma once

#include "problem/AProblem.h"

class CMyProblem : public AProblem
{
public:
    explicit CMyProblem(/* parsed template data */);

    SProblemEncoding& GetProblemEncoding() override;
    void Evaluate(AIndividual& individual) override;
    void LogSolution(AIndividual& individual) override;
    void LogAdditionalData() override;

    float GetOptimalValue() override;

private:
    SProblemEncoding m_ProblemEncoding;

    void BuildEncoding();
};
```

Example implementation outline:

```cpp
#include "CMyProblem.h"

CMyProblem::CMyProblem(/* parsed instance data */)
{
    BuildEncoding();
}

void CMyProblem::BuildEncoding()
{
    m_ProblemEncoding.m_objectivesNumber = 2;

    SEncodingSection section;
    section.m_SectionType = EEncodingType::PERMUTATION;

    // Fill section.m_SectionDescription with bounds for each decision variable.
    // The values should match the representation expected by the operators.

    m_ProblemEncoding.m_Encoding.push_back(section);
}

SProblemEncoding& CMyProblem::GetProblemEncoding()
{
    return m_ProblemEncoding;
}

void CMyProblem::Evaluate(AIndividual& individual)
{
    // Decode individual.m_Genotype.
    // Compute objective values.
    // Store objective values in individual.m_Evaluation.
    // Set individual.m_isValid = false if the solution violates hard constraints.
}

void CMyProblem::LogSolution(AIndividual& individual)
{
    // Write solution-specific details if needed.
}

void CMyProblem::LogAdditionalData()
{
    // Write extra instance or run data if needed.
}

float CMyProblem::GetOptimalValue()
{
    // Return the known optimum for single-objective benchmarks if available.
    // Otherwise leave the default behavior or return 0.0f.
    return 0.0f;
}
```

## 3. Define the problem encoding

The encoding tells iMOPSE what kind of genotype the problem expects. The available encoding types are:

```cpp
enum class EEncodingType
{
    PERMUTATION,
    BINARY,
    ASSOCIATION,
    FLOAT
};
```

Use the encoding to describe:

- how many objectives the problem has;
- which representation type is required;
- how many decision variables exist;
- what bounds are valid for each variable;
- which operators are compatible with the problem.

The encoding is used in initialization to create new solutions and must be consistent with the operators used by the selected method. For example, a permutation-based problem should be paired with initialization, crossover, and mutation operators that preserve permutation validity.

## 4. Implement factory and an instance reader

Example:

```cpp
class CMyProblemFactory
{
public:
    static CMyProblem* CreateMyProblem(const char* problemConfigurationPath);
private:
    static CMyProblemTemplate* ReadMyProblem(const char* problemConfigurationPath);
};
```

The reader should:

1. open the instance file;
2. validate the format;
3. parse all problem data;
4. construct the problem object;
5. throw a clear error if the file is invalid.


## 5. Register the problem in `CProblemFactory`

Open:

```text
optimizer/src/factories/problem/CProblemFactory.cpp
```

Add the include:

```cpp
#include "MY_PROBLEM/CMyProblemFactory.h"
```

Register the problem name in `CMyProblemFactory::CreateMyProblem(...)`:

```cpp
if (strcmp(problemName, "MY_PROBLEM") == 0)
{
    return CMyProblemFactory::CreateMyProblem(problemConfigurationPath);
}
```

The string `"MY_PROBLEM"` is the name passed as the second command-line argument:

```bash
./imopse <MethodConfigPath> MY_PROBLEM <ProblemDefinitionPath> <OutputDirectory> [ExecutionsCount] [Seed]
```

## 6. Add problem instances

Create a directory under:

```text
configurations/problems/MY_PROBLEM/
```

Example:

```text
configurations/problems/MY_PROBLEM/example.def
```

Keep the instance format documented. A useful instance file should have:

- problem size;
- objective-related data;
- constraint-related data;
- known optimum or reference value if available;
- comments explaining the format if the parser supports comments.

## 7. Run the problem

Example command:

```bash
cd optimizer/build

./imopse \
  ../../configurations/methods/MY_METHOD/MY_METHOD_MY_PROBLEM.cfg \
  MY_PROBLEM \
  ../../configurations/problems/MY_PROBLEM/example.def \
  ../experiments/MY_METHOD/example/ \
  1 \
  0
```

## Problem checklist

Before committing the problem, verify that:

- the problem class derives from `AProblem`;
- `GetProblemEncoding()` returns a fully initialized encoding;
- `Evaluate()` writes all objective values to `individual.m_Evaluation`;
- invalid solutions are handled consistently;
- `LogSolution()` produces useful output;
- `LogAdditionalData()` does not fail when no extra data is needed;
- the problem is registered in `CProblemFactory`;
- the problem name matches the command-line name exactly;
- at least one valid instance file is included;
- at least one method can run on the problem;
- generated output can be analyzed or visualized when applicable.


# Building after changes

From the optimizer directory:

```bash
cd optimizer
mkdir -p build
cd build
cmake ..
make
```

The optimizer CMake configuration recursively collects `.h` and `.cpp` files under `optimizer/src`, so files placed there are compiled automatically.

Run the executable without arguments to check that it was built correctly:

```bash
./imopse
```

Expected usage pattern:

```text
Usage: <pathToExecutable> <MethodConfigPath> <ProblemName> <ProblemDefinitionPath> <OutputDirectory> [ExecutionsCount] [Seed]
```