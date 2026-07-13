---
layout: default
title: Instance sets
permalink: /docs/instances/
nav_order: 5
---

# Instance sets

iMOPSE provides ready-to-use instance sets for several single- and multi-objective optimization problems. These instances are stored in the repository under: `configurations/problems/`.<br>
The instance files can be used directly with the optimizer as the `ProblemDefinitionPath` argument.


## MSRCPSP instances

The `configurations/problems/MSRCPSP/` directory contains multiple instance sets for the Multi-Skill Resource-Constrained Project Scheduling Problem.<br>
Most MSRCPSP instance files follow a parameterized naming pattern: `<number_of_tasks>_<number_of_resources>_<parameter_3>_<parameter_4>[_variant].def`.

Available MSRCPSP sets:

## Available MSRCPSP instance sets

<table>
  <colgroup>
    <col style="width: 10%;">
    <col style="width: 18%;">
    <col style="width: 8%;">
    <col style="width: 64%;">
  </colgroup>
  <thead>
    <tr>
      <th>Set</th>
      <th>Directory</th>
      <th>Instances</th>
      <th>Description</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td>Small</td>
      <td><code>MSRCPSP/Small/</code></td>
      <td>6</td>
      <td>Set of the smallest MSRCPSP instances - tasks: <code>10-15</code>, resources: <code>3-9</code>, precedence relations: <code>3-9</code>, skills: <code>5-12</code>.</td>
    </tr>
    <tr>
      <td>Dense</td>
      <td><code>MSRCPSP/Dense/</code></td>
      <td>7</td>
      <td>Set of dense MSRCPSP instances - tasks: <code>100</code>, resources: <code>10-40</code>, precedence relations: <code>1024-4096</code>, skills: <code>9-15</code>.</td>
    </tr>
    <tr>
      <td>NoConstr</td>
      <td><code>MSRCPSP/NoConstr/</code></td>
      <td>8</td>
      <td>Set of MSRCPSP instances without precedence constraints - tasks: <code>100-1000</code>, resources: <code>20-40</code>, precedence relations: <code>0</code>, skills: <code>1</code>.</td>
    </tr>
    <tr>
      <td>GenRegular</td>
      <td><code>MSRCPSP/GenRegular/</code></td>
      <td>100</td>
      <td>Set of generated MSRCPSP instances - tasks: <code>100-200</code>, resources: <code>5-40</code>, precedence relations: <code>64-512</code>, skills: <code>5-10</code>.</td>
    </tr>
    <tr>
      <td>GenBig</td>
      <td><code>MSRCPSP/GenBig/</code></td>
      <td>80</td>
      <td>Set of large generated MSRCPSP instances - tasks: <code>500-1000</code>, resources: <code>10-40</code>, precedence relations: <code>512-4096</code>, skills: <code>5-10</code>.</td>
    </tr>
    <tr>
      <td>d36</td>
      <td><code>MSRCPSP/d36/</code></td>
      <td>36</td>
      <td>d36 set of iMOPSE MSRCPSP instances - tasks: <code>100-200</code>, resources: <code>5-40</code>, precedence relations: <code>20-150</code>, skills: <code>9-15</code>.</td>
    </tr>
    <tr>
      <td>d45</td>
      <td><code>MSRCPSP/d45/</code></td>
      <td>45</td>
      <td>New balanced set of mixed MSRCPSP instances sampled using the novel Object Cluster Hierarchy-Guided Sampling (OCH-GS) - tasks: <code>10-1000</code>, resources: <code>3-40</code>, precedence relations: <code>0-4096</code>, skills: <code>1-15</code>.</td>
    </tr>
  </tbody>
</table>

## CVRP and ECVRPTW instances

The CVRP directory contains capacitated vehicle routing instances: `configurations/problems/CVRP/`.<br>
The CVRP instances are benchmark routing instances in `.vrp` format. They include customer coordinates, customer demands, vehicle capacity, and depot information. The available files include instances from the `A`, `M`, and `P` benchmark sets.<br>
The available CVRP instances range from small cases with `32` nodes to larger cases with up to `200` nodes.<br>
Most CVRP instance files follow this naming pattern: `<set>-n<number_of_nodes>-k<number_of_vehicles>.vrp`.

The ECVRPTW directory contains electric capacitated vehicle routing instances with time windows: `configurations/problems/ECVRPTW/`.<br>
The ECVRPTW instances are benchmark routing instances in .txt format. They are based on the Electric Vehicle Routing Problem with Time Windows and Recharging Stations Solomon benchmark instances. These instances extend vehicle routing with electric vehicle constraints, charging stations, battery capacity, and customer time windows.<br>
The available ECVRPTW instances range from cases with around `101` nodes to larger cases with up to `208` nodes.<br>
The instance names use prefixes describing the customer distribution: `c` for clustered instances, `r` for random instances, and `rc` for mixed random-clustered instances.<br>
Most ECVRPTW instance files follow one of these naming patterns: `<distribution><number>C<size>.txt or <distribution><number>_<variant>.txt`.

## TSP instances


The TSP directory contains Traveling Salesman Problem instances: `configurations/problems/TSP/`.<br>
The TSP instances are benchmark routing instances in `.tsp` format. They come from TSPLIB-style benchmark sets and include city coordinates or distance data used to define the cost of traveling between nodes.<br>
The available TSP instances range from small cases with `14` nodes to very large cases with up to `33810` nodes.<br>
Most TSP instance files follow this naming pattern: `<instance_name><number_of_nodes>.tsp`.


## TTP instances

The TTP directory contains Traveling Thief Problem instances: `configurations/problems/TTP/`.<br>
The TTP instances are benchmark instances in `.ttp` format. They combine a Traveling Salesman Problem tour with a Knapsack Problem item-selection component. The instances are based on known TSP benchmark instances and generated item sets.<br>
The available TTP instances include small test cases, selected `eil51` instances, and larger instances based on TSP instances such as `berlin52`, `kroA100`, `pr76`, and `rd100`.<br>
Most TTP instance files follow this naming pattern: `<tsp_instance>_n<number_of_items>_<item_type>_<variant>.ttp`.
