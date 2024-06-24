---
layout: default
title: Optimizer
permalink: /docs/optimizer/
nav_order: 3
---

# Optimizer
Optimizer is the main iMOPSE component responsible for problem optimization with usage of metaheuristic algorithms.
The optimizer architecture emphasizes modularity with two primary modules: the `method` module and the `problem` module, alongside additional utilities. The `method` module includes an `operators` submodule, designed for flexibility and extensibility, allowing for the incorporation of a range of optimization algorithms via a generic `AMethod` interface. The `problem` module contains implementations for various optimization problems, adhering to a `AProblem` interface that supports integration with the `method` module.

## Input parameters
The optimizer executable takes the following parameters when run:
1. **Method Configuration File:** Path to the configuration file for the optimization method. This file contains settings specific to the algorithm you wish to use, you can find more specific information in **Methods** section.
2. **Problem Name:** The name of the optimization problem you are addressing. List of available problem names is listed in **Problems** section.
3. **Problem Instance Path:** Path to the file that defines the problem instance. This includes data like distances in TSP or resources and tasks in MS-RCPSP.
4. **Output directory:** Path to directory where optimization results will be saved.
5. **Number of Runs (Optional):** The number of repetitions of the experiment. Useful for statistical analysis.
6. **Seed (Optional):** A seed value for the random number generator to ensure reproducibility of the results.
   Note: This value applies only to the first run. For each subsequent run, the seed is incremented by one.

**example:**
- Method Configuration File: `../../configurations/methods/BNTGA/BNTGA_MSRCPSP.cfg`
- Problem Name: `MSRCPSP_TA2`
- Problem Instance File: `../../configurations/problems/MSRCPSP/Regular/200_20_150_9_D5.def`
- Output Directory: `../experiments/BNTGA/200_20_150_9_D5/`
- Number of Runs: `10`
- Seed: `0`

## Architecture
The optimizer is organized into two main modules: `method` and `problem`.

- The `method` module contains implementations of metaheuristic algorithms. Within this module, there is an `operators` submodule that includes various operators which can be used across different methods for various types of problems.

- The `problem` module contains implementations of different problems. Each problem must implement the `AProblem` interface, ensuring a standardized approach to problem definition and solution.

Both methods and problems are instantiated using corresponding factory classes in the `factories` module. Each method must implement the `AMethod` interface, maintaining consistency across different algorithm implementations.

To add new problems or methods, you need to implement the appropriate interfaces (`AProblem` and `AMethod`) and extend the corresponding factory classes.

![UML Diagram](../additions/imopse_basic_class_diagram.png)