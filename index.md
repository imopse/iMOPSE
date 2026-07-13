---
layout: default
nav_order: 1
nav_exclude: true
---

# iMOPSE

**iMOPSE** - the Intelligent Multi-Objective Problem Solving Environment - is an open-source C++ library for single- and multi-objective metaheuristic optimization.

The library is designed mainly for research, benchmarking, and experimentation with NP-hard combinatorial optimization problems. It provides ready-to-use optimization methods, problem implementations, benchmark instance sets, method configuration files, and tools for analyzing optimization results.

Reproducibility is particularly important in metaheuristic optimization because algorithm performance often depends on implementation details, parameter settings, random seeds, stopping criteria, and the selected benchmark instances. Without a common environment, comparing results from different studies can be unreliable, even when the same algorithm name or problem type is used.

The motivation behind iMOPSE is to provide a unified and extensible framework for running optimization experiments in a consistent way. By collecting multiple methods, problem definitions, instance sets, configuration files, and analysis tools in one environment, iMOPSE supports repeatable experiments and makes it easier to compare new approaches with existing ones.

## What is included

iMOPSE contains two main C++ projects:

- `optimizer` - the main executable used to run optimization methods on selected problem instances.
- `paretoAnalyzer` - a tool for analyzing and comparing multi-objective optimization results.

The repository also includes ready-to-use configuration files, problem instances, and additional Python scripts for visualization and result analysis.

## Key elements

### Single- and multi-objective optimization

iMOPSE supports both single-objective and multi-objective optimization. It includes several metaheuristic methods, including evolutionary algorithms, local search methods, decomposition-based methods, swarm-based methods, ant-colony-based methods, hyper-heuristics, and hybrid approaches.

Some methods are available only for single-objective configurations, some only for multi-objective configurations, and some have configurations for both.

#### Single-objective methods

- Ant Colony Optimization - ACO
- Genetic Algorithm - GA
- Genetic Programming Hyper-Heuristic - GPHH
- Simulated Annealing - SA
- Particle Swarm Optimization - PSO
- Tabu Search - TS
- Differential Evolution - DE, LSHADE, LDEGR

#### Multi-objective methods

- Non-dominated Sorting Genetic Algorithm II - NSGA-II
- Non-dominated Tournament Genetic Algorithm 2 - NTGA2
- Strength Pareto Evolutionary Algorithm 2 - SPEA2
- Balancing Non-dominated Tournament Genetic Algorithm - BNTGA
- Adaptive Non-dominated Tournament Genetic Algorithm - ANTGA
- Multi-Objective Evolutionary Algorithm Based on Decomposition - MOEA/D
- Genetic Programming Hyper-Heuristic - GPHH
- NTGA2 with Adaptive Large Neighborhood Search - NTGA2-ALNS

### Ready-to-use benchmark problems

The library provides implementations and instance sets for several NP-hard optimization problems, including:

- Multi-Skill Resource-Constrained Project Scheduling Problem - MSRCPSP
- Traveling Salesman Problem - TSP
- Capacitated Vehicle Routing Problem - CVRP
- Electric Capacitated Vehicle Routing Problem with Time Windows - ECVRPTW
- Traveling Thief Problem - TTP

The instance files are stored under:

```text
configurations/problems/
```

### Method configuration files

Optimization methods are configured through text-based configuration files. These files define method parameters such as population size, generation limit, crossover operator, mutation operator, selection strategy, and other algorithm-specific settings.

The method configuration files are stored under:

```text
configurations/methods/
```

This makes it possible to run different methods on the same problem instance and compare their results in a reproducible way.

### Specialized operators and encodings

iMOPSE includes specialized solution encodings and operators for different problem types. For example, routing problems can use order-based crossover and route mutation operators, while scheduling problems can use operators designed for task-resource assignments or task-order representations.

This allows the library to support both general metaheuristic experimentation and problem-specific optimization.

### Result analysis tools

For multi-objective optimization, iMOPSE provides tools for Pareto front analysis and comparison. The `paretoAnalyzer` project can be used to analyze results from multiple optimization runs and compare multi-objective methods performance based on standard metrics.

Additional Python scripts are available for visualization and validation, including:

- Pareto front visualization
- Single-objective result visualization
- MSRCPSP solution visualization and validation

## Typical workflow

A typical iMOPSE experiment consists of the following steps:

1. Choose a problem instance from `configurations/problems/`.
2. Choose a method configuration from `configurations/methods/`.
3. Build the `optimizer` project.
4. Run the optimizer with the selected method and problem instance.
5. Store results in an experiment output directory.
6. Use `paretoAnalyzer` or visualization scripts to analyze the results.

The optimizer uses the following command format:

```text
<pathToExecutable> <MethodConfigPath> <ProblemName> <ProblemDefinitionPath> <OutputDirectory> [ExecutionsCount] [Seed]
```

## Who is it for?

iMOPSE is intended for researchers, students, and practitioners working with metaheuristic optimization, especially in the area of combinatorial and NP-hard problems.

It can be used to:

- compare optimization methods,
- test new operators or encodings,
- run benchmark experiments,
- analyze Pareto front approximations,
- study single- and multi-objective optimization behavior,
- prepare reproducible computational experiments.

## Citation

If you use iMOPSE library in a scientific work, We would appreciate citation to the following paper(s):

Gmyrek, Konrad and Myszkowski, Paweł B. and Antkiewicz, Michał and Olech, Łukasz P.
*iMOPSE: a Comprehensive Open Source Library for Single- and Multi-objective Metaheuristic Optimization*
2024,
https://doi.org/10.1007/978-3-031-70068-2_11,
doi: 10.1007/978-3-031-70068-2_11,
booktitle: Parallel Problem Solving from Nature – PPSN XVIII: 18th International Conference, PPSN 2024, Hagenberg, Austria, September 14–18, 2024, Proceedings, Part II,
pages: 170–184