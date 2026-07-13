---
layout: default
title: Configurations
permalink: /docs/configurations/
nav_order: 4
---

# Available method configurations

## Method configurations

Method configuration files are stored in `imopse/configurations/methods/`.

The configurations below are grouped by optimization method and divided into single-objective and multi-objective variants where applicable.

## Ant Colony Optimization

`imopse/configurations/methods/ACO/ACO_CVRP.cfg` - ACO configuration for CVRP.

## Genetic Algorithm

`imopse/configurations/methods/GA/GA_CVRP.cfg` - GA configuration for CVRP. Uses `CVRP_OX` crossover and `CVRP_Reverse_Flip` mutation.<br>
`imopse/configurations/methods/GA/GA_TSP.cfg` - GA configuration for TSP. Uses `TTP_OX_SX` crossover and `TTP_Reverse_Flip` mutation.<br>
`imopse/configurations/methods/GA/GA_TTP_SO.cfg` - GA configuration for single-objective TTP. Uses `TTP_OX_SX` crossover and `TTP_Reverse_Flip` mutation.<br>
`imopse/configurations/methods/GA/GA_MSRCPSP_2d.cfg` - GA configuration for MSRCPSP with task-association encoding. Uses `UniformCX` crossover and `RandomBit` mutation.<br>
`imopse/configurations/methods/GA/GA_MSRCPSP_TO_2d.cfg` - GA configuration for MSRCPSP with task-order permutational encoding. Uses `TTP_OX_SX` crossover, `TTP_Reverse_Flip` mutation.<br>
`imopse/configurations/methods/GA/GA_TTP_MO.cfg` - GA configuration for TTP. Uses `TTP_OX_SX` crossover, `TTP_Reverse_Flip` mutation, and objective weights `[0.3, 0.7]`.<br>

## NSGA-II

`imopse/configurations/methods/NSGAII/NSGAII_MSRCPSP.cfg` - NSGA-II configuration for MSRCPSP. Uses `UniformCX` crossover, `RandomBit` mutation, and ranked tournament selection.<br>
`imopse/configurations/methods/NSGAII/NSGAII_TTP.cfg` - NSGA-II configuration for TTP. Uses `TTP_OX_SX` crossover, `TTP_Reverse_Flip` mutation, and ranked tournament selection.<br>

## NTGA2

`imopse/configurations/methods/NTGA2/NTGA2_MSRCPSP.cfg` - Original NTGA2 configuration. Uses `UniformCX` crossover, `RandomBit` mutation and ranked tournament selection.<br>
`imopse/configurations/methods/NTGA2/NTGA2_TTP.cfg` - NTGA2 configuration for TTP. Uses `TTP_OX_SX` crossover, `TTP_Reverse_Flip` mutation and ranked tournament selection.<br>

## SPEA2

`imopse/configurations/methods/SPEA2/SPEA2_MSRCPSP.cfg` - SPEA2 configuration for MSRCPSP. Uses `UniformCX` crossover and `RandomBit` mutation.

## BNTGA


`imopse/configurations/methods/BNTGA/BNTGA_ECVRPTW.cfg` - BNTGA configuration for ECVRPTW. Uses `ECVRPTW` initialization, `CVRP_OX` crossover and `CVRP_Reverse_Flip` mutation.<br>
`imopse/configurations/methods/BNTGA/BNTGA_MSRCPSP.cfg` - BNTGA configuration for MSRCPSP. Uses `UniformCX` crossover and `RandomBit` mutation.<br>
`imopse/configurations/methods/BNTGA/BNTGA_TTP.cfg` - BNTGA configuration for TTP. Uses `TTP_OX_SX` crossover and `TTP_Reverse_Flip` mutation.<br>

## ANTGA

`imopse/configurations/methods/ANTGA/ANTGA_MSRCPSP.cfg` - ANTGA configuration for MSRCPSP. Uses `UniformCX` crossover, base `RandomBit` mutation, and `UniformMultiOperator` with `RandomBit` sub-mutation.<br>
`imopse/configurations/methods/ANTGA/ANTGA_MSRCPSP_3mut.cfg` - ANTGA configuration for MSRCPSP using three sub-mutations: `RandomBit`, `CheapestResourceMutation`, and `LeastAssignedResourceMutation`.<br>
`imopse/configurations/methods/ANTGA/ANTGA_MSRCPSP_base.cfg` - Base ANTGA configuration for MSRCPSP. Uses `UniformCX` crossover and `UniformMultiOperator` with `RandomBit` sub-mutation.<br>
`imopse/configurations/methods/ANTGA/ANTGA_MSRCPSP_cost.cfg` - ANTGA configuration for cost-focused MSRCPSP optimization. Uses `UniformCX` crossover and `UniformMultiOperator` with `CheapestResourceMutation`.<br>
`imopse/configurations/methods/ANTGA/ANTGA_MSRCPSP_makespan.cfg` - ANTGA configuration for makespan-focused MSRCPSP optimization. Uses `UniformCX` crossover and `UniformMultiOperator` with `LeastAssignedResourceMutation`.<br>

## MOEA/D

`imopse/configurations/methods/MOEAD/MOEAD_MSRCPSP.cfg` - MOEA/D configuration for MSRCPSP. Uses `UniformCX` crossover, `RandomBit` mutation, `PartitionsNumber 2`, and `NeighbourhoodSize 2`.<br>
`imopse/configurations/methods/MOEAD/MOEAD_TTP.cfg` - MOEA/D configuration for TTP. Uses `TTP_OX_SX` crossover, `TTP_Reverse_Flip` mutation, `PartitionsNumber 100`, and `NeighbourhoodSize 3`.<br>

## Genetic Programming Hyper-Heuristic

`imopse/configurations/methods/GPHH/GPHH_CVRP.cfg` - GPHH configuration for CVRP.<br>
`imopse/configurations/methods/GPHH/GPHH_MSRCPSP.cfg` - GPHH configuration for MSRCPSP.<br>

## Differential Evolution

`imopse/configurations/methods/DEGR/DE_MSRCPSP_2d.cfg` - Differential Evolution configuration for MSRCPSP with Greedy Schedule Builder.<br>

## Particle Swarm Optimization

`imopse/configurations/methods/PSO/PSO_MSRCPSP_2d.cfg` - PSO configuration for MSRCPSP.<br>

## Simulated Annealing

`imopse/configurations/methods/SA/SA_TTP.cfg` - SA configuration for TTP.<br>
`imopse/configurations/methods/SA/SA_MSRCPSP_2d.cfg` - SA configuration for MSRCPSP.<br>

## Tabu Search

`imopse/configurations/methods/TS/TS_TTP.cfg` - TS configuration for TTP.<br>
`imopse/configurations/methods/TS/TS_MSRCPSP_2d.cfg` - TS configuration for MSRCPSP.<br>

## NTGA2-ALNS

`imopse/configurations/methods/NTGA2_ALNS/NTGA2_ALNS.cfg` - Hybrid NTGA2-ALNS configuration. Uses `CVRP_OX` crossover, `CVRP_Reverse_Flip` mutation and ranked tournament selection.<br>
`imopse/configurations/methods/NTGA2_ALNS/NTGA2_ALNS_small.cfg` - Smaller hybrid NTGA2-ALNS configuration. Uses `CVRP_OX` crossover, `CVRP_Reverse_Flip` mutation and ranked tournament selection.<br>

# Available problems

Problem name have to be provided as one of input parameters.

## Multi-Skill Resource-Constrained Project Scheduling Problem

`MSRCPSP_TA` - Task-resource association-based solution encoding with greedy order of task execution for all 5 objectives.<br>
`MSRCPSP_TA2` - Task-resource association-based solution encoding with two objectives: makespan and cost.<br>
`MSRCPSP_TA_FLOAT` - Task-resource association-based solution encoding with greedy order of task execution as normalized float representation.<br>
`MSRCPSP_TO` - Order permutation-based solution encoding with greedy task-resource association for all 5 objectives.<br>
`MSRCPSP_TO2` - Order permutation-based solution encoding with two objectives: makespan and cost.<br>
`MSRCPSP_TO_FLOAT` - Order permutation-based solution encoding with greedy task-resource association as normalized float priority list representation.<br>

## Traveling Salesman Problem

`TSP` - Traveling Salesman Problem with permutation-based encoding.

## Capacitated Vehicle Routing Problem

`CVRP` - Capacitated Vehicle Routing Problem with permutation-based encoding.<br>
`ECVRPTW` - Electric Capacitated Vehicle Routing Problem with Time Windows with permutation-based encoding.<br>

## Traveling Thief Problem

`TTP1` - Single-objective Traveling Thief Problem with renting ratio for aggregating objectives with permutation-based encoding.<br>
`TTP2` - Multi-objective Traveling Thief Problem with permutation-based encoding.<br>
