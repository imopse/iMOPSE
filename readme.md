# iMOPSE

The Intelligent Multi-Objective Problem Solving Environment is an open-source C++ library that specializes
in single- and multi-objective metaheuristic optimization especially in the area of NP-hard optimization problems.
Our library provides a set of optimization algorithms, problems and ready-to-use problem instances and tools useful in research and benchmarking.

### Citation policy

If you use iMOPSE library in a scientific work, We would appreciate citation to the following paper(s):

@inproceedings{imopse-lib,
author = {Gmyrek, Konrad and Myszkowski, Pawe\l{} B. and Antkiewicz, Micha\l{} and Olech, \L{}ukasz P.},
title = {iMOPSE: a Comprehensive Open Source Library for Single- and Multi-objective Metaheuristic Optimization},
year = {2024},
url = {https://doi.org/10.1007/978-3-031-70068-2_11},
doi = {10.1007/978-3-031-70068-2_11},
booktitle = {Parallel Problem Solving from Nature – PPSN XVIII: 18th International Conference, PPSN 2024, Hagenberg, Austria, September 14–18, 2024, Proceedings, Part II},
pages = {170–184}
}


### Key Features
- **State-of-the-art Optimization Methods**:
  - Multi-Objective Evolutionary Algorithm Based on Decomposition (MOEAD)
  - Non-dominated Sorting Genetic Algorithm II (NSGAII)
  - Non-dominated Tournament Genetic Algorithm 2 (NTGA2)
  - Strength Pareto Evolutionary Algorithm 2 (SPEA2)
  - Balancing Non-dominated Tournament Genetic Algorithm (BNTGA)
- **NP-hard Problems** with ready-to-use **instances**:
  - Multi-Skill Resource-Constrained Project Scheduling Problem (MSRPCP)
  - Traveling Salesman Problem (TSP)
  - Traveling Thief Problem (TTP)
  - Capacitated Vehicle Routing Problem (CVRP)
- **Flexible Encoding Mechanism** and **Specialized Operators**:  iMOPSE excels in handling classical NP-hard problems with constraints, with focus on scheduling and combinatorial problems like MS-RCPSP or TSP. Its versatile encoding system effectively manages various problems and enhances the application of specialized operators.
- **Comprehensive Tool Suite**: iMOPSE provides pre-configured problem instances and method setups, complemented by an extensive array of tools for data collection, visualization, and analysis, making it highly effective for research and result interpretation.
- **Extensive Customization**: Offers numerous customization options to facilitate research into diverse optimization methods and scenarios.
- **User-Friendly Interface**: Features an intuitive interface with straightforward input parameters and configuration files, making it accessible to a wide audience of users.

# Compile project
To start working with iMOPSE, clone the repository, it consists of two C++ projects `optimizer` and `paretoAnalyzer`, both of them contain CMakeLists.txt files.
In this subsection we present a few example methods to compile C++ project.
## using CMake
- Verify CMake and Make 
  Verify if CMake and Make are installed on your system. If not, you will need to install them. Visit the [CMake Website](https://cmake.org) and the [GNU Make Manual](https://www.gnu.org/software/make/manual/make.html#Installing-Make) for installation instructions.
- Enter optimizer directory, create new `build` directory and enter it
```bash
cd optimizer
mkdir build
cd build
```
- Run CMake
```bash
cmake ..
```
- Compile the project
```bash
make
```
- Run executable
```bash
./imopse
```
Expected output should be: `Usage: <pathToExecutable> <MethodConfigPath> <ProblemName> <ProblemDefinitionPath> <OutputDirectory> [ExecutionsCount] [Seed]`

## using Clion IDE
- Open project in CLion.
- In one of project directories find `CMakeLists.txt`, right-click it and select `Load CMake Project` option.
- CLion will automatically build the project.
- After building the run button will be available in the top-right corner. Expected output should be: `Usage: <pathToExecutable> <MethodConfigPath> <ProblemName> <ProblemDefinitionPath> [ExecutionsCount] [Seed]`

# Optimizer
Optimizer is the main iMOPSE component responsible for problem optimization with usage of metaheuristic algorithms.
The optimizer architecture emphasizes modularity with two primary modules: the `method` module and the `problem` module, alongside additional utilities. The `method` module includes an `operators` submodule, designed for flexibility and extensibility, allowing for the incorporation of a range of optimization algorithms via a generic `AMethod` interface. The `problem` module contains implementations for various optimization problems, adhering to a `AProblem` interface that supports integration with the `method` module.

## Input parameters
The optimizer executable takes the following parameters when run:
1. **Method Configuration Path:** Path to the configuration file for the optimization method. This file contains settings specific to the algorithm you wish to use.
2. **Problem Name:** The name of the optimization problem you are addressing. List of available problem names is listed below.
3. **Problem Definition Path:** Path to the file that defines the problem instance. This includes data like distances in TSP or resources and tasks in MS-RCPSP.
4. **Output directory:** Path to directory where optimization results will be saved.
5. **Executions Count (Optional):** The number of times the optimization should be run. Useful for statistical analysis.
6. **Seed (Optional):** A seed value for the random number generator to ensure reproducibility of the results.
Note: This value applies only to the first run. For each subsequent run, the seed is incremented by one.

List of possible to input problem names:
- **Multi-Skill Resource-Constrained Project Scheduling Problem**:
  - **MSRCPSP_TA**: Task-resource association-based solution encoding and greedy order of tasks execution
  - **MSRCPSP_TA2**: Task-resource association-based solution encoding and greedy order of tasks execution, with only two objectives (makespan, cost)
  - **MSRCPSP_TO**: Order permutation-based solution with greedy task association
  - **MSRCPSP_TO2**: Order permutation-based solution with greedy task association, with only two objectives (makespan, cost)
- **Traveling Salesman Problem**: TSP
- **Traveling Thief Problem**:
  - **TTP1**: Single-objective TTP
  - **TTP2**: Multi-objective TTP
- **Capacitated Vehicle Routing Problem**: CVRP

## Architecture
The optimizer is organized into two main modules: `method` and `problem`.

- The `method` module contains implementations of metaheuristic algorithms. Within this module, there is an `operators` submodule that includes various operators which can be used across different methods for various types of problems.

- The `problem` module contains implementations of different problems. Each problem must implement the `AProblem` interface, ensuring a standardized approach to problem definition and solution.

Both methods and problems are instantiated using corresponding factory classes in the `factories` module. Each method must implement the `AMethod` interface, maintaining consistency across different algorithm implementations.

To add new problems or methods, you need to implement the appropriate interfaces (`AProblem` and `AMethod`) and extend the corresponding factory classes.

![UML Diagram](additions/imopse_basic_class_diagram.png)

# Pareto Analyzer

## Input Parameters
The `paretoAnalyzer` executable requires the following parameters:
1. **Config File Path:** Path to the configuration file containing paths to result directories.
2. **Instance Name:** The specific instance to analyze, used for filtering result data.
3. **Output Directory:** Directory where Pareto Front Approximation files will be saved.

## Features
The `ParetoAnalyzer` project is essential for analyzing and comparing multi-objective optimization results. It offers the following features:
- **True Pareto Front Calculation:** Generates the best possible Pareto Front Approximation using results from all runs of compared methods.
- **Nadir Point Calculation:** Determines the worst possible values for all objectives.
- **Pareto Visualization:** Uses Python scripts to visualize Pareto Front Approximations, offering clear insights into optimization outcomes.
- **Quality Measures for Multi-Objective Optimization:** Calculates metrics such as Inverted Generational Distance (IGD), HyperVolume (HV), Pareto Front Size (PFS), and Purity to assess the quality of Pareto Front Approximations.

# Additional Tools
iMOPSE includes various tools for visualization, and validation, enhancing its research capabilities. These additional tools are implemented as Python scripts:

- **msrcpsp_solution_visualizer:** Validates and visualizes MS-RCPSP solutions.
- **multi-objective_visualizer:** Visualizes trade-offs between competing objectives for multi-objective optimization.
- **single-objective_visualizer:** Provides a graphical overview of fitness values for single-objective optimization.

# Example of Use
This section provides instructions on how to use iMOPSE to compare two methods, BNTGA and MOEAD, on the MSRCPSP problem.

First, specify and optionally edit the method configuration file and other parameters as listed below:

**input parameters for BNTGA:**
- Method Configuration File: `../../configurations/methods/BNTGA/BNTGA_MSRCPSP.cfg`
- Problem Name: `MSRCPSP_TA2`
- Problem Instance File: `../../configurations/problems/MSRCPSP/Regular/200_20_150_9_D5.def`
- Output Directory: `../experiments/BNTGA/200_20_150_9_D5/`
- Number of Runs: `10`
- Seed: `0`

**input parameters for MOEAD:**
- Method Configuration File: `../../configurations/methods/MOEAD/MOEAD_MSRCPSP.cfg`
- Problem Name: `MSRCPSP_TA2`
- Problem Instance File: `../../configurations/problems/MSRCPSP/Regular/200_20_150_9_D5.def`
- Output Directory: `../experiments/MOEAD/200_20_150_9_D5/`
- Number of Runs: `10`
- Seed: `0`

Here, we use `BNTGA_MSRCPSP.cfg` and `MOEAD_MSRCPSP.cfg` configuration files for the methods, as well as `MSRCPSP_TA2` described previously. The instance is from the Regular set and is named `200_20_150_9_D5`. We also specify the output directory with the instance name, which will be helpful later during Pareto analysis. Additionally, we specify the repetition number.

Configuration Files:

**BNTGA Configuration File:**

MethodName BNTGA  
GenerationLimit 100  
Crossover UniformCX 0.6  
Mutation RandomBit 0.01  
PopulationSize 50  
GapSelection 40  

**MOEAD Configuration File:**

MethodName MOEAD  
GenerationLimit 1000  
Crossover UniformCX 0.6  
Mutation RandomBit 0.01  
PartitionsNumber 50  
NeighbourhoodSize 10  

During the optimization run for both methods, the optimizer outputs results to the specified directories, each containing Pareto Front Approximation from every run.

Now, we use Pareto Analyzer to calculate metrics and Python scripts for creating a visual comparison. For this, create a configuration file named `config.cfg`:

../../optimizer/experiments/BNTGA  
../../optimizer/experiments/MOEAD


Run the optimizer providing the following arguments:
- Configuration file: `../config.cfg`
- Instance name: `200_20_150_9_D5`
- Result directory: `../results/`

The analysis produces the following results:

| Metric        | BNTGA                       | MOEAD                        |
|---------------|-----------------------------|------------------------------|
| Runs          | 10                          | 10                           |
| TPFS          | 731                         | 107                          |
| MPFS          | 731                         | 107                          |
| MND           | 731                         | 0                            |
| HV            | 0.83860                     | 0.66322                      |
| HV Std        | 0.04535                     | 0.02192                      |
| GD            | 0.00177                     | 0.01216                      |
| GD Std        | 0.00317                     | 0.00283                      |
| IGD           | 0.00070                     | 0.01147                      |
| IGD Std       | 0.00102                     | 0.00137                      |
| PFS           | 435.20001                   | 64.10000                     |
| PFS Std       | 207.27750                   | 9.57549                      |
| ND            | 263.20001                   | 0.00000                      |
| ND Std        | 266.12546                   | 0.00000                      |
| ND/TPFS       | 0.36005                     | 0.00000                      |
| ND/TPFS Std   | 0.36406                     | 0.00000                      |

Based on the metrics provided, we can observe and compare the performance of BNTGA and MOEAD on the MSRCPSP problem.

Using the generated files, we can compare Pareto Front Approximations from each method.

![UML Diagram](additions/BNTGA_MOEAD_comparison.png)

On plot we can visually compare quality of each method result.

# Support
For support, feature requests, or bug reports, please file an issue through the GitHub issue tracker associated with the project.

# Authors and Acknowledgment
Thanks to all the contributors who have invested their time and expertise in developing iMOPSE.

# Contributors
## Current
- **Paweł Myszkowski** (2011 - Now)
- **Michał Antkiewicz** (2021 - Now)
- **Konrad Gmyrek** (2023 - Now)
- **Łukasz Olech** (2023 - Now)

## Past Collaborators
- **Adam Krzeminski** (2023 - 2024)
- **Adrian Żak** (2023 - 2024)
- **Maciej Laszczyk** (2014 - 2021)
- **Kamil Król** (2021 - 2022)
- **Jacek Wernikowski** (2014 - 2016)
- **Jakub Graniczny** (2018 - 2020)
- **Kamil Dziadek** (2018 - 2019)
- **Joanna Lichodij** (2017 - 2018)
- **Ivan Nikulin** (2015 - 2016)
- **Dawid Kalinowski** (2017 - 2018)
- **Marek Skowroński** (2011 - 2015)
- **Jędrzej Siemieński** (2014 - 2015)
- **Krzysztof Oślizło** (2012 - 2013)
- **Marcin Adamski** (2013 - 2014)
- **Paweł Kwiatek** (2011 - 2012)
- **Łukasz Podlodowski** (2011 - 2012)
- **Kacper Małkowski** (2024)
- **Jakub Korycki** (2024)
