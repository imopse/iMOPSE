---
layout: default
title: Pareto Analyzer
permalink: /docs/pareto_analyzer/
---

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
